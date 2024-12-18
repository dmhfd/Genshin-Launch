#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
struct ImageInfo
{
    int width, length;
};
struct Group
{
    int wide, high;
    ImageInfo one;
};
struct GIF
{
    IWICImagingFactory *factory;
    IWICBitmapDecoder *pDecoder;
    ID2D1HwndRenderTarget *pRenderTarget;
    UINT length = 0, height = 0, currentx = 0, currenty = 0, counter = 0, m_cFrames;
    ID2D1Bitmap *pBitmap;
    IWICStream *pStream;
    HRSRC imageResHandle;
    HGLOBAL imageResDataHandle;
    void *pImageFile;
    DWORD imageFileSize;
    RECT rc;
    D2D1_SIZE_U size;
    D2D1_SIZE_F size2;
    PROPVARIANT propValue;
    BOOL Transform = 0;
};
struct createparam
{
    unsigned ResID, timeID, delay;
    GIF *gif;
};
struct MakePtr
{
    LONG_PTR a;
    LONG_PTR b;
};