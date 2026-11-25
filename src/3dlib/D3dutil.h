
















#ifndef D3DUTIL_H
#define D3DUTIL_H






#define SAFE_DELETE(p)  { if (p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }











HRESULT D3DUtil_SetProjectionMatrix(D3DMATRIX& mat, FLOAT fFOV = 1.570795f,
                                     FLOAT fAspect = 1.0f,
                                     FLOAT fNearPlane = 1.0f,
                                     FLOAT fFarPlane = 1000.0f);

inline VOID D3DUtil_SetIdentityMatrix(D3DMATRIX& m)
{
    m._12 = m._13 = m._14 = m._21 = m._23 = m._24 = 0.0f;
    m._31 = m._32 = m._34 = m._41 = m._42 = m._43 = 0.0f;
    m._11 = m._22 = m._33 = m._44 = 1.0f;
}







HRESULT _DbgOut(TCHAR*, DWORD, HRESULT, TCHAR*);

#if defined(DEBUG) || defined(_DEBUG)
    #define DEBUG_MSG(str)    _DbgOut(__FILE__, (DWORD)__LINE__, 0, str)
    #define DEBUG_ERR(hr,str) _DbgOut(__FILE__, (DWORD)__LINE__, hr, str)
#else
    #define DEBUG_MSG(str)    (0L)
    #define DEBUG_ERR(hr,str) (hr)
#endif


#endif
