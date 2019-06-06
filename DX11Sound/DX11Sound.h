#ifndef DX11_SOUND_H
#define DX11_SOUND_H


#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <stdio.h>

#ifdef DLL
    #ifdef EXPORT
        #define MYPROJECT_API __declspec(dllexport)
    #else
        #define MYPROJECT_API __declspec(dllimport)
    #endif
#else
    #define MYPROJECT_API 
#endif


class MYPROJECT_API DX11Sound
{
private:
	struct WaveHeaderType
	{
		char chunkId[4];
		unsigned long chunkSize;
		char format[4];
		char subChunkId[4];
		unsigned long subChunkSize;
		unsigned short audioFormat;
		unsigned short numChannels;
		unsigned long sampleRate;
		unsigned long bytesPerSecond;
		unsigned short blockAlign;
		unsigned short bitsPerSample;
		char dataChunkId[4];
		unsigned long dataSize;
	};

public:
	DX11Sound();
	~DX11Sound();

	bool Initialize(HWND);
	void Shutdown();

private:
	bool InitializeDirectSound(HWND);
	void ShutdownDirectSound();

	bool LoadWaveFile(char*, IDirectSoundBuffer8**);
	void ShutdownWaveFile(IDirectSoundBuffer8**);

	bool PlayWaveFile();

private:
	IDirectSound8* m_DirectSound;
	IDirectSoundBuffer* m_primaryBuffer;
	IDirectSoundBuffer8* m_secondaryBuffer1;
};

#endif // DX11_SOUND_H