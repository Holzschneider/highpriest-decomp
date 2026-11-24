// Headless query against the kept Ghidra project of the reference executable.
// Run through port/tools/refexe.sh, which passes the arguments:
//
//   <out-file> <mode> <arg> [disasm|decomp|both] [max-functions] [exact|substr]
//
//   mode string  functions that reference a string containing (or equal to) <arg>
//   mode addr    the function containing address <arg> (hex, with or without 0x)
//   mode func    the function named <arg> (a Ghidra name such as FUN_00599b00
//                or a name applied from anchors.tsv)
//   mode xrefs   the functions that call the function containing address <arg>
//   mode anchors apply names from the anchors file <arg> (needs a writable open)
//
// Output goes to <out-file> so that the shell wrapper can print it without
// Ghidra's log lines.
//@category Bifrost

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.StringDataInstance;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.Symbol;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class RefexeQuery extends GhidraScript {

	private PrintWriter out;
	private String view = "decomp";
	private int maxFunctions = 8;
	private DecompInterface decomp;

	@Override
	protected void run() throws Exception {
		String[] a = getScriptArgs();
		if (a.length < 3) {
			printerr("usage: <out-file> <mode> <arg> [view] [max] [exact|substr]");
			return;
		}
		out = new PrintWriter(a[0], StandardCharsets.UTF_8);
		try {
			String mode = a[1], arg = a[2];
			if (a.length > 3) view = a[3];
			if (a.length > 4) maxFunctions = Integer.parseInt(a[4]);
			boolean exact = a.length > 5 && a[5].equals("exact");
			switch (mode) {
			case "string": byString(arg, exact); break;
			case "addr": byAddress(arg); break;
			case "func": byName(arg); break;
			case "xrefs": callers(arg); break;
			case "anchors": applyAnchors(arg); break;
			default: out.println("unknown mode " + mode);
			}
		} finally {
			if (decomp != null) decomp.dispose();
			out.close();
		}
	}

	private void byString(String text, boolean exact) throws Exception {
		Map<Function, List<String>> hits = new LinkedHashMap<>();
		int strings = 0;
		DataIterator it = currentProgram.getListing().getDefinedData(true);
		while (it.hasNext() && !monitor.isCancelled()) {
			Data d = it.next();
			if (!d.hasStringValue()) continue;
			String s = StringDataInstance.getStringDataInstance(d).getStringValue();
			if (s == null) continue;
			if (exact ? !s.equals(text) : !s.contains(text)) continue;
			strings++;
			Reference[] refs = getReferencesTo(d.getAddress());
			out.printf("string @ %s \"%s\": %d reference(s)%n", d.getAddress(), escape(s), refs.length);
			for (Reference r : refs) {
				Function f = getFunctionContaining(r.getFromAddress());
				String where = r.getFromAddress().toString();
				if (f == null) {
					out.printf("  from %s (no function)%n", where);
					continue;
				}
				out.printf("  from %s in %s @ %s%n", where, f.getName(true), f.getEntryPoint());
				hits.computeIfAbsent(f, k -> new ArrayList<>()).add(where);
			}
		}
		if (strings == 0) {
			out.println("no defined string " + (exact ? "equal to" : "containing") + " \"" + text + "\"");
			return;
		}
		out.println();
		int n = 0;
		for (Map.Entry<Function, List<String>> e : hits.entrySet()) {
			if (n++ >= maxFunctions) {
				out.printf("... %d more function(s); raise --max to see them%n", hits.size() - maxFunctions);
				break;
			}
			show(e.getKey(), "references the string at " + String.join(", ", e.getValue()));
		}
	}

	private void byAddress(String hex) throws Exception {
		Address addr = toAddr(hex.startsWith("0x") ? hex.substring(2) : hex);
		Function f = getFunctionContaining(addr);
		if (f == null) {
			out.println("no function contains " + addr);
			return;
		}
		show(f, "contains " + addr);
	}

	private void byName(String name) throws Exception {
		List<Function> fs = new ArrayList<>();
		for (Function f : currentProgram.getFunctionManager().getFunctions(true))
			if (f.getName(true).equals(name) || f.getName().equals(name)) fs.add(f);
		if (fs.isEmpty()) {
			out.println("no function named " + name);
			return;
		}
		for (Function f : fs) show(f, "named " + name);
	}

	private void callers(String hex) throws Exception {
		Address addr = toAddr(hex.startsWith("0x") ? hex.substring(2) : hex);
		Function f = getFunctionContaining(addr);
		if (f == null) {
			out.println("no function contains " + addr);
			return;
		}
		out.printf("callers of %s @ %s:%n", f.getName(), f.getEntryPoint());
		for (Reference r : getReferencesTo(f.getEntryPoint())) {
			Function c = getFunctionContaining(r.getFromAddress());
			out.printf("  %s %s in %s%n", r.getReferenceType(), r.getFromAddress(),
				c == null ? "(no function)" : c.getName() + " @ " + c.getEntryPoint());
		}
	}

	// anchors.tsv: address <tab> name <tab> evidence. Lines starting with # are skipped.
	private void applyAnchors(String path) throws Exception {
		try (BufferedReader r = new BufferedReader(new FileReader(path, StandardCharsets.UTF_8))) {
			String line;
			while ((line = r.readLine()) != null) {
				if (line.isBlank() || line.startsWith("#")) continue;
				String[] f = line.split("\t");
				if (f.length < 2) continue;
				Address addr = toAddr(f[0].replaceFirst("^0x", ""));
				Function fn = getFunctionAt(addr);
				if (fn == null) fn = createFunction(addr, null);
				if (fn == null) {
					out.println("no function at " + addr + " for " + f[1]);
					continue;
				}
				// Ghidra names cannot hold "::"; the class becomes a namespace.
				String name = f[1];
				int sep = name.lastIndexOf("::");
				if (sep > 0) {
					ghidra.program.model.symbol.Namespace ns = getNamespace(null, name.substring(0, sep));
					if (ns == null) ns = currentProgram.getSymbolTable().createClass(
						currentProgram.getGlobalNamespace(), name.substring(0, sep), SourceType.USER_DEFINED);
					fn.setParentNamespace(ns);
					name = name.substring(sep + 2);
				}
				fn.setName(name, SourceType.USER_DEFINED);
				out.println("named " + addr + " " + f[1]);
			}
		}
	}

	private void show(Function f, String why) throws Exception {
		out.println("==== " + f.getName(true) + " @ " + f.getEntryPoint() + " (" + f.getBody().getNumAddresses()
			+ " bytes; " + why + ")");
		if (!view.equals("decomp")) {
			InstructionIterator ii = currentProgram.getListing().getInstructions(f.getBody(), true);
			while (ii.hasNext()) {
				Instruction ins = ii.next();
				String comment = "";
				for (Reference r : ins.getReferencesFrom()) {
					Data d = getDataAt(r.getToAddress());
					if (d != null && d.hasStringValue())
						comment = "  ; \"" + escape(String.valueOf(d.getValue())) + "\"";
					else {
						Function t = getFunctionAt(r.getToAddress());
						if (t != null) comment = "  ; " + t.getName(true);
					}
				}
				out.printf("  %s  %s%s%n", ins.getAddress(), ins, comment);
			}
		}
		if (!view.equals("disasm")) {
			if (decomp == null) {
				decomp = new DecompInterface();
				DecompileOptions opts = new DecompileOptions();
				opts.setMaxPayloadMBytes(256); // the packet dispatchers are 40 KB of code
				decomp.setOptions(opts);
				decomp.openProgram(currentProgram);
			}
			DecompileResults res = decomp.decompileFunction(f, 120, monitor);
			if (res.decompileCompleted())
				out.println(res.getDecompiledFunction().getC());
			else
				out.println("decompilation failed: " + res.getErrorMessage());
		}
		out.println();
	}

	private static String escape(String s) {
		return s.replace("\\", "\\\\").replace("\n", "\\n").replace("\r", "\\r").replace("\"", "\\\"");
	}
}
