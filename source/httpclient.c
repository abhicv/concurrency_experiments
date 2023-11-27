#include <Windows.h>
#include <winhttp.h>
#include <stdio.h>

typedef struct ByteArray
{
    char *data;
    unsigned int size;
} ByteArray;

void PushIntoByteArray(ByteArray *array, char *sourceBuffer, unsigned int sourceBufSize)
{
    if(sourceBuffer == 0) return;
    if(sourceBufSize == 0) return;

    if(array->size == 0) 
    {
        array->data = (char*)calloc(sourceBufSize, sizeof(char));
        memcpy(array->data, sourceBuffer, sourceBufSize);
        array->size = sourceBufSize;
    }
    else 
    {
        array->data = (char*)realloc(array->data, array->size + sourceBufSize);
        memcpy(array->data + array->size, sourceBuffer, sourceBufSize);
        array->size += sourceBufSize;
    }
}

ByteArray DoGetRequest(LPCWSTR url)
{
    URL_COMPONENTS urlComponents = {0};
    urlComponents.dwStructSize = sizeof(URL_COMPONENTS);
    urlComponents.dwSchemeLength = (DWORD)-1;
    urlComponents.dwHostNameLength = (DWORD)-1;
    urlComponents.dwUrlPathLength = (DWORD)-1;
    urlComponents.dwExtraInfoLength = (DWORD)-1;

    WinHttpCrackUrl(url, 0, 0, &urlComponents);

    WCHAR *hostname = calloc(urlComponents.dwHostNameLength + 1, sizeof(WCHAR));
    wcsncpy(hostname, urlComponents.lpszHostName, urlComponents.dwHostNameLength);

    HINTERNET sessionHandle = WinHttpOpen(L"WINHTTP client/1.0",
                                    WINHTTP_ACCESS_TYPE_NO_PROXY,
                                    WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS, 0);

    HINTERNET conHandle = WinHttpConnect(sessionHandle,
                                         hostname,
                                         INTERNET_DEFAULT_HTTPS_PORT, 0);

    HINTERNET requestHandle = WinHttpOpenRequest(conHandle, L"GET", urlComponents.lpszUrlPath,
                                                 0, WINHTTP_NO_REFERER,
                                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                 WINHTTP_FLAG_SECURE);
    
    // DWORD options = WINHTTP_DECOMPRESSION_FLAG_ALL;
    // BOOL result = WinHttpSetOption(requestHandle, WINHTTP_OPTION_DECOMPRESSION, &options, sizeof(DWORD));
    // BOOL error = GetLastError();

    WinHttpSendRequest(requestHandle,
                       WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                       WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    WinHttpReceiveResponse(requestHandle, 0);

    DWORD size = 0;
    
    if(!WinHttpQueryHeaders(requestHandle, WINHTTP_QUERY_RAW_HEADERS_CRLF, 0, 0, &size, 0) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        LPWSTR buffer = malloc(size);
        if(WinHttpQueryHeaders(requestHandle, WINHTTP_QUERY_RAW_HEADERS_CRLF, 0, buffer, &size, 0)) 
        {
            wprintf(L"%s", buffer);
        }
        free(buffer);
    }

    DWORD error = GetLastError();
    
    ByteArray array = {0};

    DWORD responseReadSize = 0;

    #define BUFFER_SIZE 8 * 1024

    while (TRUE)
    {
        unsigned char readBuffer[BUFFER_SIZE] = {0};
        WinHttpReadData(requestHandle, (LPVOID)readBuffer, BUFFER_SIZE, &responseReadSize);

        if(responseReadSize == 0) break;

        PushIntoByteArray(&array, readBuffer, responseReadSize);
    }

    if(requestHandle) CloseHandle(requestHandle);
    if(conHandle) CloseHandle(conHandle);
    if(sessionHandle) CloseHandle(sessionHandle);

    return array;
}

int main(int argc, char **argv)
{
    // LPCWSTR url = L"https://reqres.in/api/users?page=2";
    // LPCWSTR url = L"https://www.google.com";
    // LPCWSTR url =  L"https://raw.githubusercontent.com/json-iterator/test-data/master/large-file.json";
    // LPCWSTR url = L"https://filesamples.com/samples/image/bmp/sample_640%C3%97426.bmp";
    LPCWSTR url = L"https://images.all-free-download.com/images/graphiclarge/small_mouse_macro_515329.jpg";
    
    ByteArray array = DoGetRequest(url);

    // for(int n = 0; n < array.size; n++) 
    // {
    //     printf("%0.2x (%c),", array.data[n], array.data[n]);
    //     // printf("%0.2x ", array.data[n]);
    // }

    // printf("Length: %d\n", array.size);
    FILE *file = fopen("donwload.bin", "wb+");
    fwrite(array.data, sizeof(char), array.size, file);
    fclose(file);
    return 0;
}