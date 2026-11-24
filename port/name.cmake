# The application's name and bundle identifier: the one place they are
# defined. CMake projects include this file and pass BF_APP_NAME to the
# compiler as a definition; port/tools/flags.sh reads it for the scripts.
# Paths are built from it at run time, never written out.
set(BF_APP_NAME "Bifrost")
set(BF_BUNDLE_ID "com.example.bifrost")
