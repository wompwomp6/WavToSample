#include <Windows.h>
#include <stdio.h>
#include <inttypes.h>

typedef struct File_t
{
    BYTE* data;
    SIZE_T size;
} File;

BYTE* LoadFile(const char* path)
{
    HANDLE hFile = CreateFileA(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;

    LARGE_INTEGER fileSize;

    if (!GetFileSizeEx(hFile, &fileSize))
    {
        CloseHandle(hFile);
        return NULL;
    }

    if (fileSize.QuadPart <= 0 ||
        (ULONGLONG)fileSize.QuadPart > (ULONGLONG)SIZE_MAX)
    {
        CloseHandle(hFile);
        return NULL;
    }

    BYTE* file = (BYTE*)malloc((SIZE_T)fileSize.QuadPart);

    if (!file)
    {
        CloseHandle(hFile);
        return NULL;
    }

    DWORD bytesRead = 0;

    BOOL result = ReadFile(
        hFile,
        file,
        (DWORD)fileSize.QuadPart,
        &bytesRead,
        NULL
    );

    CloseHandle(hFile);

    if (!result || bytesRead != fileSize.QuadPart)
    {
        free(file);
        return NULL;
    }

    return file;
}

typedef struct Riff_t
{
    char ChunkID[4];
    DWORD ChunkSize;
    char Format[4];
}Riff_t;

typedef struct Fmt_t
{
    char SubChunk1ID[3];
    DWORD SubChunk1Size;
    WORD AudioFormat;
    WORD NumChannels;
    DWORD SampleRate;
    DWORD ByteRate;
    WORD BlockAlign;
    WORD BitsPerSample;
}Fmt_t;

typedef struct Data_t
{
    char SubChunk2ID[3];
    DWORD SubChunk2Size;
}Data_t;

typedef struct Wav_t
{
    Riff_t riff;
    Fmt_t fmt;
    Data_t data;
    char buff[0];
}Wav_t;

BOOL IsValidWav(Wav_t* wav)
{
    Riff_t* rif = &wav->riff;
    Fmt_t* fmt = &wav->fmt;
    Data_t* data = &wav->data;

    if (memcmp(rif->ChunkID, "RIFF", sizeof(rif->ChunkID)))
        return FALSE;

    if (memcmp(rif->Format, "WAVE", sizeof(rif->Format)))
        return FALSE;

    if (memcmp(fmt->SubChunk1ID, "fmt", sizeof(fmt->SubChunk1ID)))
        return FALSE;

    if (memcmp(data->SubChunk2ID, "data", sizeof(data->SubChunk2ID)))
        return FALSE;

    //if (rif->ChunkSize != 4 + (8 + fmt->SubChunk1Size) + (8 + data->SubChunk2Size))
    //    return FALSE;
    
    if (fmt->ByteRate != fmt->SampleRate * fmt->NumChannels * fmt->BitsPerSample / 8)
        return FALSE;
    
    if (fmt->AudioFormat != 1) // PCM
        return FALSE;

    if (fmt->NumChannels != 1)
        return FALSE;
    
    if (fmt->BitsPerSample != 8)
        return FALSE;
    
    if (fmt->SampleRate != 8000)
        return FALSE;

    if (fmt->BitsPerSample / 8 != wav->fmt.BlockAlign)
        return FALSE;

    return TRUE;
}

void PrintWav(Wav_t* wav)
{
    DWORD sampleCount = wav->data.SubChunk2Size / wav->fmt.BlockAlign;

    printf("unsigned char sample[] = {\n   ");

    for (size_t i = 0; i < sampleCount; i++)
    {
        printf("0x%02hhX, ", wav->buff[i]);
        if (((i + 1) % 8) == 0)
            printf("\n   ");
    }

    printf("\n};");
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Please pass the file path as the second arg\n");
        return EXIT_FAILURE;
    }

    BYTE* file = LoadFile(argv[1]);

    if (!file)
    {
        printf("Failed to load file.\n");
        return EXIT_FAILURE;
    }

    if (!IsValidWav((Wav_t*)file))
    {
        printf("Please enter a valid .wav file with mono 8bit pcm at 8000hz\n");
        return EXIT_FAILURE;
    }

    PrintWav((Wav_t*)file);

    while (!GetAsyncKeyState(VK_END));

    free(file);

    return EXIT_SUCCESS;
}
