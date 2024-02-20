//
//  wave_file.cpp
//
//
//  Created by Manuel Cabrerizo on 16/02/2024.
//

//this struct is the minimal required header data for a wav file
struct WaveFileHeader
{
	//the main chunk
	unsigned char m_szChunkID[4];
	uint32 m_nChunkSize;
	unsigned char m_szFormat[4];

	//sub chunk 1 "fmt "
	unsigned char m_szSubChunk1ID[4];
	uint32 m_nSubChunk1Size;
	uint16 m_nAudioFormat;
	uint16 m_nNumChannels;
	uint32 m_nSampleRate;
	uint32 m_nByteRate;
	uint16 m_nBlockAlign;
	uint16 m_nBitsPerSample;

	//sub chunk 2 "data"
	unsigned char m_szSubChunk2ID[4];
	uint32 m_nSubChunk2Size;

	//then comes the data!
};

bool WriteWaveFile(const char *szFileName, void *pData, int32 nDataSize, int16 nNumChannels, int32 nSampleRate, int32 nBitsPerSample)
{
	//open the file if we can
	FILE *File = fopen(szFileName,"w+b");
	if(!File)
	{
		return false;
	}

	WaveFileHeader waveHeader;

	//fill out the main chunk
	memcpy(waveHeader.m_szChunkID,"RIFF",4);
	waveHeader.m_nChunkSize = nDataSize + 36;
	memcpy(waveHeader.m_szFormat,"WAVE",4);

	//fill out sub chunk 1 "fmt "
	memcpy(waveHeader.m_szSubChunk1ID,"fmt ",4);
	waveHeader.m_nSubChunk1Size = 16;
	waveHeader.m_nAudioFormat = 1;
	waveHeader.m_nNumChannels = nNumChannels;
	waveHeader.m_nSampleRate = nSampleRate;
	waveHeader.m_nByteRate = nSampleRate * nNumChannels * nBitsPerSample / 8;
	waveHeader.m_nBlockAlign = nNumChannels * nBitsPerSample / 8;
	waveHeader.m_nBitsPerSample = nBitsPerSample;

	//fill out sub chunk 2 "data"
	memcpy(waveHeader.m_szSubChunk2ID,"data",4);
	waveHeader.m_nSubChunk2Size = nDataSize;

	//write the header
	fwrite(&waveHeader,sizeof(WaveFileHeader),1,File);

	//write the wave data itself
	fwrite(pData,nDataSize,1,File);

	//close the file and return success
	fclose(File);
	return true;
}

// TODO: stream the audio file ...
MacSoundStream LoadWavFile(Arena *arena, const char *szFileName) {

    FILE *file = fopen(szFileName, "rb");
    if(!file) {
        MacSoundStream zero = {};
        return zero;
    }
    // go to the end of the file
    fseek(file, 0, SEEK_END);
    // get the size of the file to alloc the memory we need
    long int fileSize = ftell(file);
    // go back to the start of the file
    fseek(file, 0, SEEK_SET);
    // alloc the memory
    uint8 *wavData = (uint8 *)ArenaPushSize(arena, fileSize + 1);
    memset(wavData, 0, fileSize + 1);
    // store the content of the file
    fread(wavData, fileSize, 1, file);
    wavData[fileSize] = '\0'; // null terminating string...
    fclose(file);

   
    WaveFileHeader *header = (WaveFileHeader *)wavData;
    void *data = (wavData + sizeof(WaveFileHeader)); 

    MacSoundStream stream;
    stream.data = data;
    stream.size = header->m_nSubChunk2Size;
    return stream;
}

