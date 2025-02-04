// WAV audio loader and writer. Public domain. See "unlicense" statement at the end of this file.
// dr_wav - v0.9.0 - 2018-12-16
//
// David Reid - mackron@gmail.com

/*
DEPRECATED APIS
===============
Version 0.9.0 deprecated the per-sample reading and seeking APIs and replaced them with versions that work on the resolution
of a PCM frame instead. For example, given a stereo WAV file, previously you would pass 2 to drwav_read_f32() to read one
PCM frame, whereas now you would pass in 1 to drwav_read_pcm_frames_f32(). The old APIs would return the number of samples
read, whereas now it will return the number of PCM frames. Below is a list of APIs that have been deprecated and their
replacements.

	drwav_read()                     -> drwav_read_pcm_frames()
	drwav_read_s16()                 -> drwav_read_pcm_frames_s16()
	drwav_read_f32()                 -> drwav_read_pcm_frames_f32()
	drwav_read_s32()                 -> drwav_read_pcm_frames_s32()
	drwav_seek_to_sample()           -> drwav_seek_to_pcm_frame()
	drwav_write()                    -> drwav_write_pcm_frames()
	drwav_open_and_read_s16()        -> drwav_open_and_read_pcm_frames_s16()
	drwav_open_and_read_f32()        -> drwav_open_and_read_pcm_frames_f32()
	drwav_open_and_read_s32()        -> drwav_open_and_read_pcm_frames_s32()
	drwav_open_file_and_read_s16()   -> drwav_open_file_and_read_pcm_frames_s16()
	drwav_open_file_and_read_f32()   -> drwav_open_file_and_read_pcm_frames_f32()
	drwav_open_file_and_read_s32()   -> drwav_open_file_and_read_pcm_frames_s32()
	drwav_open_memory_and_read_s16() -> drwav_open_memory_and_read_pcm_frames_s16()
	drwav_open_memory_and_read_f32() -> drwav_open_memory_and_read_pcm_frames_f32()
	drwav_open_memory_and_read_s32() -> drwav_open_memory_and_read_pcm_frames_s32()
	drwav::totalSampleCount          -> drwav::totalPCMFrameCount

Rationale:
	1) Most programs will want to read in multiples of the channel count which demands a per-frame reading API. Per-sample
	   reading just adds complexity and maintenance costs for no practical benefit.
	2) This is consistent with my other decoders - dr_flac and dr_mp3.

These APIs will be removed completely in version 0.10.0. You can continue to use drwav_read_raw() if you need per-sample
reading.
*/

// USAGE
//
// This is a single-file library. To use it, do something like the following in one .c file.
//     #define DR_WAV_IMPLEMENTATION
//     #include "dr_wav.h"
//
// You can then #include this file in other parts of the program as you would with any other header file. Do something
// like the following to read audio data:
//
//     drwav wav;
//     if (!drwav_init_file(&wav, "my_song.wav")) {
//         // Error opening WAV file.
//     }
//
//     drwav_int32* pDecodedInterleavedSamples = malloc(wav.totalPCMFrameCount * wav.channels * sizeof(drwav_int32));
//     size_t numberOfSamplesActuallyDecoded = drwav_read_pcm_frames_s32(&wav, wav.totalPCMFrameCount, pDecodedInterleavedSamples);
//
//     ...
//
//     drwav_uninit(&wav);
//
// You can also use drwav_open() to allocate and initialize the loader for you:
//
//     drwav* pWav = drwav_open_file("my_song.wav");
//     if (pWav == NULL) {
//         // Error opening WAV file.
//     }
//
//     ...
//
//     drwav_close(pWav);
//
// If you just want to quickly open and read the audio data in a single operation you can do something like this:
//
//     unsigned int channels;
//     unsigned int sampleRate;
//     drwav_uint64 totalPCMFrameCount;
//     float* pSampleData = drwav_open_file_and_read_pcm_frames_f32("my_song.wav", &channels, &sampleRate, &totalPCMFrameCount);
//     if (pSampleData == NULL) {
//         // Error opening and reading WAV file.
//     }
//
//     ...
//
//     drwav_free(pSampleData);
//
// The examples above use versions of the API that convert the audio data to a consistent format (32-bit signed PCM, in
// this case), but you can still output the audio data in its internal format (see notes below for supported formats):
//
//     size_t samplesRead = drwav_read_pcm_frames(&wav, wav.totalPCMFrameCount, pDecodedInterleavedSamples);
//
// You can also read the raw bytes of audio data, which could be useful if dr_wav does not have native support for
// a particular data format:
//
//     size_t bytesRead = drwav_read_raw(&wav, bytesToRead, pRawDataBuffer);
//
//
// dr_wav can also be used to output WAV files. This does not currently support compressed formats. To use this, look at
// drwav_open_write(), drwav_open_file_write(), etc. Use drwav_write_pcm_frames() to write samples, or drwav_write_raw()
// to write raw data in the "data" chunk.
//
//     drwav_data_format format;
//     format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
//     format.format = DR_WAVE_FORMAT_PCM;          // <-- Any of the DR_WAVE_FORMAT_* codes.
//     format.channels = 2;
//     format.sampleRate = 44100;
//     format.bitsPerSample = 16;
//     drwav* pWav = drwav_open_file_write("data/recording.wav", &format);
//
//     ...
//
//     drwav_uint64 samplesWritten = drwav_write_pcm_frames(pWav, frameCount, pSamples);
//
//
// dr_wav has seamless support the Sony Wave64 format. The decoder will automatically detect it and it should Just Work
// without any manual intervention.
//
//
// OPTIONS
// #define these options before including this file.
//
// #define DR_WAV_NO_CONVERSION_API
//   Disables conversion APIs such as drwav_read_pcm_frames_f32() and drwav_s16_to_f32().
//
// #define DR_WAV_NO_STDIO
//   Disables drwav_open_file(), drwav_open_file_write(), etc.
//
//
//
// QUICK NOTES
// - Samples are always interleaved.
// - The default read function does not do any data conversion. Use drwav_read_pcm_frames_f32(), drwav_read_pcm_frames_s32()
//   and drwav_read_pcm_frames_s16() to read and convert audio data to 32-bit floating point, signed 32-bit integer and
//   signed 16-bit integer samples respectively. Tested and supported internal formats include the following:
//   - Unsigned 8-bit PCM
//   - Signed 12-bit PCM
//   - Signed 16-bit PCM
//   - Signed 24-bit PCM
//   - Signed 32-bit PCM
//   - IEEE 32-bit floating point
//   - IEEE 64-bit floating point
//   - A-law and u-law
//   - Microsoft ADPCM
//   - IMA ADPCM (DVI, format code 0x11)
// - dr_wav will try to read the WAV file as best it can, even if it's not strictly conformant to the WAV format.


#ifndef dr_wav_h
#define dr_wav_h

#include <stddef.h>

#if defined(_MSC_VER) && _MSC_VER < 1600
typedef   signed char    drwav_int8;
typedef unsigned char    drwav_uint8;
typedef   signed short   drwav_int16;
typedef unsigned short   drwav_uint16;
typedef   signed int     drwav_int32;
typedef unsigned int     drwav_uint32;
typedef   signed __int64 drwav_int64;
typedef unsigned __int64 drwav_uint64;
#else
#include <stdint.h>
typedef int8_t           drwav_int8;
typedef uint8_t          drwav_uint8;
typedef int16_t          drwav_int16;
typedef uint16_t         drwav_uint16;
typedef int32_t          drwav_int32;
typedef uint32_t         drwav_uint32;
typedef int64_t          drwav_int64;
typedef uint64_t         drwav_uint64;
#endif
typedef drwav_uint8      drwav_bool8;
typedef drwav_uint32     drwav_bool32;
#define DRWAV_TRUE       1
#define DRWAV_FALSE      0

#ifdef __cplusplus
extern "C" {
#endif

	// Common data formats.
#define DR_WAVE_FORMAT_PCM          0x1
#define DR_WAVE_FORMAT_ADPCM        0x2
#define DR_WAVE_FORMAT_IEEE_FLOAT   0x3
#define DR_WAVE_FORMAT_ALAW         0x6
#define DR_WAVE_FORMAT_MULAW        0x7
#define DR_WAVE_FORMAT_DVI_ADPCM    0x11
#define DR_WAVE_FORMAT_EXTENSIBLE   0xFFFE

// Constants.
#ifndef DRWAV_MAX_SMPL_LOOPS
#define DRWAV_MAX_SMPL_LOOPS        1
#endif

// Flags to pass into drwav_init_ex(), etc.
#define DRWAV_SEQUENTIAL            0x00000001

	typedef enum
	{
		drwav_seek_origin_start,
		drwav_seek_origin_current
	} drwav_seek_origin;

	typedef enum
	{
		drwav_container_riff,
		drwav_container_w64
	} drwav_container;

	typedef struct
	{
		union
		{
			drwav_uint8 fourcc[4];
			drwav_uint8 guid[16];
		} id;

		// The size in bytes of the chunk.
		drwav_uint64 sizeInBytes;

		// RIFF = 2 byte alignment.
		// W64  = 8 byte alignment.
		unsigned int paddingSize;
	} drwav_chunk_header;

	// Callback for when data is read. Return value is the number of bytes actually read.
	//
	// pUserData   [in]  The user data that was passed to drwav_init(), drwav_open() and family.
	// pBufferOut  [out] The output buffer.
	// bytesToRead [in]  The number of bytes to read.
	//
	// Returns the number of bytes actually read.
	//
	// A return value of less than bytesToRead indicates the end of the stream. Do _not_ return from this callback until
	// either the entire bytesToRead is filled or you have reached the end of the stream.
	typedef size_t(*drwav_read_proc)(void* pUserData, void* pBufferOut, size_t bytesToRead);

	// Callback for when data is written. Returns value is the number of bytes actually written.
	//
	// pUserData    [in]  The user data that was passed to drwav_init_write(), drwav_open_write() and family.
	// pData        [out] A pointer to the data to write.
	// bytesToWrite [in]  The number of bytes to write.
	//
	// Returns the number of bytes actually written.
	//
	// If the return value differs from bytesToWrite, it indicates an error.
	typedef size_t(*drwav_write_proc)(void* pUserData, const void* pData, size_t bytesToWrite);

	// Callback for when data needs to be seeked.
	//
	// pUserData [in] The user data that was passed to drwav_init(), drwav_open() and family.
	// offset    [in] The number of bytes to move, relative to the origin. Will never be negative.
	// origin    [in] The origin of the seek - the current position or the start of the stream.
	//
	// Returns whether or not the seek was successful.
	//
	// Whether or not it is relative to the beginning or current position is determined by the "origin" parameter which
	// will be either drwav_seek_origin_start or drwav_seek_origin_current.
	typedef drwav_bool32(*drwav_seek_proc)(void* pUserData, int offset, drwav_seek_origin origin);

	// Callback for when drwav_init_ex/drwav_open_ex finds a chunk.
	//
	// pChunkUserData    [in] The user data that was passed to the pChunkUserData parameter of drwav_init_ex(), drwav_open_ex() and family.
	// onRead            [in] A pointer to the function to call when reading.
	// onSeek            [in] A pointer to the function to call when seeking.
	// pReadSeekUserData [in] The user data that was passed to the pReadSeekUserData parameter of drwav_init_ex(), drwav_open_ex() and family.
	// pChunkHeader      [in] A pointer to an object containing basic header information about the chunk. Use this to identify the chunk.
	//
	// Returns the number of bytes read + seeked.
	//
	// To read data from the chunk, call onRead(), passing in pReadSeekUserData as the first parameter. Do the same
	// for seeking with onSeek(). The return value must be the total number of bytes you have read _plus_ seeked.
	//
	// You must not attempt to read beyond the boundary of the chunk.
	typedef drwav_uint64(*drwav_chunk_proc)(void* pChunkUserData, drwav_read_proc onRead, drwav_seek_proc onSeek, void* pReadSeekUserData, const drwav_chunk_header* pChunkHeader);

	// Structure for internal use. Only used for loaders opened with drwav_open_memory().
	typedef struct
	{
		const drwav_uint8* data;
		size_t dataSize;
		size_t currentReadPos;
	} drwav__memory_stream;

	// Structure for internal use. Only used for writers opened with drwav_open_memory_write().
	typedef struct
	{
		void** ppData;
		size_t* pDataSize;
		size_t dataSize;
		size_t dataCapacity;
		size_t currentWritePos;
	} drwav__memory_stream_write;

	typedef struct
	{
		drwav_container container;  // RIFF, W64.
		drwav_uint32 format;        // DR_WAVE_FORMAT_*
		drwav_uint32 channels;
		drwav_uint32 sampleRate;
		drwav_uint32 bitsPerSample;
	} drwav_data_format;

	typedef struct
	{
		// The format tag exactly as specified in the wave file's "fmt" chunk. This can be used by applications
		// that require support for data formats not natively supported by dr_wav.
		drwav_uint16 formatTag;

		// The number of channels making up the audio data. When this is set to 1 it is mono, 2 is stereo, etc.
		drwav_uint16 channels;

		// The sample rate. Usually set to something like 44100.
		drwav_uint32 sampleRate;

		// Average bytes per second. You probably don't need this, but it's left here for informational purposes.
		drwav_uint32 avgBytesPerSec;

		// Block align. This is equal to the number of channels * bytes per sample.
		drwav_uint16 blockAlign;

		// Bits per sample.
		drwav_uint16 bitsPerSample;

		// The size of the extended data. Only used internally for validation, but left here for informational purposes.
		drwav_uint16 extendedSize;

		// The number of valid bits per sample. When <formatTag> is equal to WAVE_FORMAT_EXTENSIBLE, <bitsPerSample>
		// is always rounded up to the nearest multiple of 8. This variable contains information about exactly how
		// many bits a valid per sample. Mainly used for informational purposes.
		drwav_uint16 validBitsPerSample;

		// The channel mask. Not used at the moment.
		drwav_uint32 channelMask;

		// The sub-format, exactly as specified by the wave file.
		drwav_uint8 subFormat[16];
	} drwav_fmt;

	typedef struct
	{
		drwav_uint32 cuePointId;
		drwav_uint32 type;
		drwav_uint32 start;
		drwav_uint32 end;
		drwav_uint32 fraction;
		drwav_uint32 playCount;
	} drwav_smpl_loop;

	typedef struct
	{
		drwav_uint32 manufacturer;
		drwav_uint32 product;
		drwav_uint32 samplePeriod;
		drwav_uint32 midiUnityNotes;
		drwav_uint32 midiPitchFraction;
		drwav_uint32 smpteFormat;
		drwav_uint32 smpteOffset;
		drwav_uint32 numSampleLoops;
		drwav_uint32 samplerData;
		drwav_smpl_loop loops[DRWAV_MAX_SMPL_LOOPS];
	} drwav_smpl;

	typedef struct
	{
		// A pointer to the function to call when more data is needed.
		drwav_read_proc onRead;

		// A pointer to the function to call when data needs to be written. Only used when the drwav object is opened in write mode.
		drwav_write_proc onWrite;

		// A pointer to the function to call when the wav file needs to be seeked.
		drwav_seek_proc onSeek;

		// The user data to pass to callbacks.
		void* pUserData;


		// Whether or not the WAV file is formatted as a standard RIFF file or W64.
		drwav_container container;


		// Structure containing format information exactly as specified by the wav file.
		drwav_fmt fmt;

		// The sample rate. Will be set to something like 44100.
		drwav_uint32 sampleRate;

		// The number of channels. This will be set to 1 for monaural streams, 2 for stereo, etc.
		drwav_uint16 channels;

		// The bits per sample. Will be set to something like 16, 24, etc.
		drwav_uint16 bitsPerSample;

		// Equal to fmt.formatTag, or the value specified by fmt.subFormat if fmt.formatTag is equal to 65534 (WAVE_FORMAT_EXTENSIBLE).
		drwav_uint16 translatedFormatTag;

		// The total number of PCM frames making up the audio data.
		drwav_uint64 totalPCMFrameCount;


		// The size in bytes of the data chunk.
		drwav_uint64 dataChunkDataSize;

		// The position in the stream of the first byte of the data chunk. This is used for seeking.
		drwav_uint64 dataChunkDataPos;

		// The number of bytes remaining in the data chunk.
		drwav_uint64 bytesRemaining;


		// Only used in sequential write mode. Keeps track of the desired size of the "data" chunk at the point of initialization time. Always
		// set to 0 for non-sequential writes and when the drwav object is opened in read mode. Used for validation.
		drwav_uint64 dataChunkDataSizeTargetWrite;

		// Keeps track of whether or not the wav writer was initialized in sequential mode.
		drwav_bool32 isSequentialWrite;


		// smpl chunk.
		drwav_smpl smpl;


		// A hack to avoid a DRWAV_MALLOC() when opening a decoder with drwav_open_memory().
		drwav__memory_stream memoryStream;
		drwav__memory_stream_write memoryStreamWrite;

		// Generic data for compressed formats. This data is shared across all block-compressed formats.
		struct
		{
			drwav_uint64 iCurrentSample;    // The index of the next sample that will be read by drwav_read_*(). This is used with "totalSampleCount" to ensure we don't read excess samples at the end of the last block.
		} compressed;

		// Microsoft ADPCM specific data.
		struct
		{
			drwav_uint32 bytesRemainingInBlock;
			drwav_uint16 predictor[2];
			drwav_int32  delta[2];
			drwav_int32  cachedSamples[4];  // Samples are stored in this cache during decoding.
			drwav_uint32 cachedSampleCount;
			drwav_int32  prevSamples[2][2]; // The previous 2 samples for each channel (2 channels at most).
		} msadpcm;

		// IMA ADPCM specific data.
		struct
		{
			drwav_uint32 bytesRemainingInBlock;
			drwav_int32  predictor[2];
			drwav_int32  stepIndex[2];
			drwav_int32  cachedSamples[16]; // Samples are stored in this cache during decoding.
			drwav_uint32 cachedSampleCount;
		} ima;


		drwav_uint64 totalSampleCount;  // <-- DEPRECATED. Will be removed in a future version.
	} drwav;


	// Initializes a pre-allocated drwav object.
	//
	// pWav                         [out]          A pointer to the drwav object being initialized.
	// onRead                       [in]           The function to call when data needs to be read from the client.
	// onSeek                       [in]           The function to call when the read position of the client data needs to move.
	// onChunk                      [in, optional] The function to call when a chunk is enumerated at initialized time.
	// pUserData, pReadSeekUserData [in, optional] A pointer to application defined data that will be passed to onRead and onSeek.
	// pChunkUserData               [in, optional] A pointer to application defined data that will be passed to onChunk.
	// flags                        [in, optional] A set of flags for controlling how things are loaded.
	//
	// Returns true if successful; false otherwise.
	//
	// Close the loader with drwav_uninit().
	//
	// This is the lowest level function for initializing a WAV file. You can also use drwav_init_file() and drwav_init_memory()
	// to open the stream from a file or from a block of memory respectively.
	//
	// If you want dr_wav to manage the memory allocation for you, consider using drwav_open() instead. This will allocate
	// a drwav object on the heap and return a pointer to it.
	//
	// Possible values for flags:
	//   DRWAV_SEQUENTIAL: Never perform a backwards seek while loading. This disables the chunk callback and will cause this function
	//                     to return as soon as the data chunk is found. Any chunks after the data chunk will be ignored.
	//
	// drwav_init() is equivalent to "drwav_init_ex(pWav, onRead, onSeek, NULL, pUserData, NULL, 0);".
	//
	// The onChunk callback is not called for the WAVE or FMT chunks. The contents of the FMT chunk can be read from pWav->fmt
	// after the function returns.
	//
	// See also: drwav_init_file(), drwav_init_memory(), drwav_uninit()
	drwav_bool32 drwav_init(drwav* pWav, drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData);
	drwav_bool32 drwav_init_ex(drwav* pWav, drwav_read_proc onRead, drwav_seek_proc onSeek, drwav_chunk_proc onChunk, void* pReadSeekUserData, void* pChunkUserData, drwav_uint32 flags);

	// Initializes a pre-allocated drwav object for writing.
	//
	// onWrite   [in]           The function to call when data needs to be written.
	// onSeek    [in]           The function to call when the write position needs to move.
	// pUserData [in, optional] A pointer to application defined data that will be passed to onWrite and onSeek.
	//
	// Returns true if successful; false otherwise.
	//
	// Close the writer with drwav_uninit().
	//
	// This is the lowest level function for initializing a WAV file. You can also use drwav_init_file() and drwav_init_memory()
	// to open the stream from a file or from a block of memory respectively.
	//
	// If the total sample count is known, you can use drwav_init_write_sequential(). This avoids the need for dr_wav to perform
	// a post-processing step for storing the total sample count and the size of the data chunk which requires a backwards seek.
	//
	// If you want dr_wav to manage the memory allocation for you, consider using drwav_open() instead. This will allocate
	// a drwav object on the heap and return a pointer to it.
	//
	// See also: drwav_init_file_write(), drwav_init_memory_write(), drwav_uninit()
	drwav_bool32 drwav_init_write(drwav* pWav, const drwav_data_format* pFormat, drwav_write_proc onWrite, drwav_seek_proc onSeek, void* pUserData);
	drwav_bool32 drwav_init_write_sequential(drwav* pWav, const drwav_data_format* pFormat, drwav_uint64 totalSampleCount, drwav_write_proc onWrite, void* pUserData);

	// Uninitializes the given drwav object.
	//
	// Use this only for objects initialized with drwav_init().
	void drwav_uninit(drwav* pWav);


	// Opens a wav file using the given callbacks.
	//
	// onRead    [in]           The function to call when data needs to be read from the client.
	// onSeek    [in]           The function to call when the read position of the client data needs to move.
	// pUserData [in, optional] A pointer to application defined data that will be passed to onRead and onSeek.
	//
	// Returns null on error.
	//
	// Close the loader with drwav_close().
	//
	// You can also use drwav_open_file() and drwav_open_memory() to open the stream from a file or from a block of
	// memory respectively.
	//
	// This is different from drwav_init() in that it will allocate the drwav object for you via DRWAV_MALLOC() before
	// initializing it.
	//
	// See also: drwav_init(), drwav_open_file(), drwav_open_memory(), drwav_close()
	drwav* drwav_open(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData);
	drwav* drwav_open_ex(drwav_read_proc onRead, drwav_seek_proc onSeek, drwav_chunk_proc onChunk, void* pReadSeekUserData, void* pChunkUserData, drwav_uint32 flags);

	// Opens a wav file for writing using the given callbacks.
	//
	// onWrite   [in]           The function to call when data needs to be written.
	// onSeek    [in]           The function to call when the write position needs to move.
	// pUserData [in, optional] A pointer to application defined data that will be passed to onWrite and onSeek.
	//
	// Returns null on error.
	//
	// Close the loader with drwav_close().
	//
	// You can also use drwav_open_file_write() and drwav_open_memory_write() to open the stream from a file or from a block
	// of memory respectively.
	//
	// This is different from drwav_init_write() in that it will allocate the drwav object for you via DRWAV_MALLOC() before
	// initializing it.
	//
	// See also: drwav_open_file_write(), drwav_open_memory_write(), drwav_close()
	drwav* drwav_open_write(const drwav_data_format* pFormat, drwav_write_proc onWrite, drwav_seek_proc onSeek, void* pUserData);
	drwav* drwav_open_write_sequential(const drwav_data_format* pFormat, drwav_uint64 totalSampleCount, drwav_write_proc onWrite, void* pUserData);

	// Uninitializes and deletes the the given drwav object.
	//
	// Use this only for objects created with drwav_open().
	void drwav_close(drwav* pWav);


	// Reads raw audio data.
	//
	// This is the lowest level function for reading audio data. It simply reads the given number of
	// bytes of the raw internal sample data.
	//
	// Consider using drwav_read_pcm_frames_s16(), drwav_read_pcm_frames_s32() or drwav_read_pcm_frames_f32() for
	// reading sample data in a consistent format.
	//
	// Returns the number of bytes actually read.
	size_t drwav_read_raw(drwav* pWav, size_t bytesToRead, void* pBufferOut);

	// Reads a chunk of audio data in the native internal format.
	//
	// This is typically the most efficient way to retrieve audio data, but it does not do any format
	// conversions which means you'll need to convert the data manually if required.
	//
	// If the return value is less than <framesToRead> it means the end of the file has been reached or
	// you have requested more samples than can possibly fit in the output buffer.
	//
	// This function will only work when sample data is of a fixed size and uncompressed. If you are
	// using a compressed format consider using drwav_read_raw() or drwav_read_pcm_frames_s16/s32/f32/etc().
	drwav_uint64 drwav_read_pcm_frames(drwav* pWav, drwav_uint64 framesToRead, void* pBufferOut);

	// Seeks to the given PCM frame.
	//
	// Returns true if successful; false otherwise.
	drwav_bool32 drwav_seek_to_pcm_frame(drwav* pWav, drwav_uint64 targetFrameIndex);


	// Writes raw audio data.
	//
	// Returns the number of bytes actually written. If this differs from bytesToWrite, it indicates an error.
	size_t drwav_write_raw(drwav* pWav, size_t bytesToWrite, const void* pData);

	// Writes PCM frames.
	//
	// Returns the number of PCM frames written.
	drwav_uint64 drwav_write_pcm_frames(drwav* pWav, drwav_uint64 framesToWrite, const void* pData);


	//// Conversion Utilities ////
#ifndef DR_WAV_NO_CONVERSION_API

// Reads a chunk of audio data and converts it to signed 16-bit PCM samples.
//
// Returns the number of PCM frames actually read.
//
// If the return value is less than <framesToRead> it means the end of the file has been reached.
	drwav_uint64 drwav_read_pcm_frames_s16(drwav* pWav, drwav_uint64 framesToRead, drwav_int16* pBufferOut);

	// Low-level function for converting unsigned 8-bit PCM samples to signed 16-bit PCM samples.
	void drwav_u8_to_s16(drwav_int16* pOut, const drwav_uint8* pIn, size_t sampleCount);

	// Low-level function for converting signed 24-bit PCM samples to signed 16-bit PCM samples.
	void drwav_s24_to_s16(drwav_int16* pOut, const drwav_uint8* pIn, size_t sampleCount);

	// Low-level function for converting signed 32-bit PCM samples to signed 16-bit PCM samples.
	void drwav_s32_to_s16(drwav_int16* pOut, const drwav_int32* pIn, size_t sampleCount);

	// Low-level function for converting IEEE 32-bit floating point samples to signed 16-bit PCM samples.
	void drwav_f32_to_s16(drwav_int16* pOut, const float* pIn, size_t sampleCount);

	// Low-level function for converting IEEE 64-bit floating point samples to signed 16-bit PCM samples.
	void drwav_f64_to_s16(drwav_int16* pOut, const double* pIn, size_t sampleCount);

	// Low-level function for converting A-law samples to signed 16-bit PCM samples.
	void drwav_alaw_to_s16(drwav_int16* pOut, const drwav_uint8* pIn, size_t sampleCount);

	// Low-level function for converting u-law samples to signed 16-bit PCM samples.
	void drwav_mulaw_to_s16(drwav_int16* pOut, const drwav_uint8* pIn, size_t sampleCount);



	// Reads a chunk of audio data and converts it to IEEE 32-bit floating point samples.
	//
	// Returns the number of PCM frames actually read.
	//
	// If the return value is less than <framesToRead> it means the end of the file has been reached.
	drwav_uint64 drwav_read_pcm_frames_f32(drwav* pWav, drwav_uint64 framesToRead, float* pBufferOut);

	// Low-level function for converting unsigned 8-bit PCM samples to IEEE 32-bit floating point samples.
	void drwav_u8_to_f32(float* pOut, const drwav_uint8* pIn, size_t sampleCount);

	// Low-level function for converting signed 16-bit PCM samples to IEEE 32-bit floating point samples.
	void drwav_s16_to_f32(float* pOut, const drwav_int16* pIn, size_t sampleCount);

	// Low-level function for converting signed 24-bit PCM samples to IEEE 32-bit floating point samples.
	void drwav_s24_to_f32(float* pOut, const drwav_uint8* pIn, size_t sampleCount);

	// Low-level function for converting signed 32-bit PCM samples to IEEE 32-bit floating point samples.
	void drwav_s32_to_f32(float* pOut, const drwav_int32* pIn, size_t sampleCount);

	// Low-level function for converting IEEE 64-bit floating point samples to IEEE 32-bit floating point samples.
	void drwav_f64_to_f32(float* pOut, const double* pIn, size_t sampleCount);

	// Low-level function for converting A-law samples to IEEE 32-bit floating point samples.
	void drwav_alaw_to_f32(float* pOut, const drwav_uint8* pIn, size_t sampleCount);

	// Low-level function for converting u-law samples to IEEE 32-bit floating point samples.
	void drwav_mulaw_to_f32(float* pOut, const drwav_uint8* pIn, size_t sampleCount);



	// Reads a chunk of audio data and converts it to signed 32-bit PCM samples.
	//
	// Returns the number of PCM frames actually read.
	//
	// If the return value is less than <framesToRead> it means the end of the file has been reached.
	drwav_uint64 drwav_read_pcm_frames_s32(drwav* pWav, drwav_uint64 framesToRead, drwav_int32* pBufferOut);

	// Low-level function for converting unsigned 8-bit PCM samples to signed 32-bit PCM samples.
	void drwav_u8_to_s32(drwav_int32* pOut, const drwav_uint8* pIn, size_t sampleCount);

	// Low-level function for converting signed 16-bit PCM samples to signed 32-bit PCM samples.
	void drwav_s16_to_s32(drwav_int32* pOut, const drwav_int16* pIn, size_t sampleCount);

	// Low-level function for converting signed 24-bit PCM samples to signed 32-bit PCM samples.
	void drwav_s24_to_s32(drwav_int32* pOut, const drwav_uint8* pIn, size_t sampleCount);

	// Low-level function for converting IEEE 32-bit floating point samples to signed 32-bit PCM samples.
	void drwav_f32_to_s32(drwav_int32* pOut, const float* pIn, size_t sampleCount);

	// Low-level function for converting IEEE 64-bit floating point samples to signed 32-bit PCM samples.
	void drwav_f64_to_s32(drwav_int32* pOut, const double* pIn, size_t sampleCount);

	// Low-level function for converting A-law samples to signed 32-bit PCM samples.
	void drwav_alaw_to_s32(drwav_int32* pOut, const drwav_uint8* pIn, size_t sampleCount);

	// Low-level function for converting u-law samples to signed 32-bit PCM samples.
	void drwav_mulaw_to_s32(drwav_int32* pOut, const drwav_uint8* pIn, size_t sampleCount);

#endif  //DR_WAV_NO_CONVERSION_API


	//// High-Level Convenience Helpers ////

#ifndef DR_WAV_NO_STDIO

// Helper for initializing a wave file using stdio.
//
// This holds the internal FILE object until drwav_uninit() is called. Keep this in mind if you're caching drwav
// objects because the operating system may restrict the number of file handles an application can have open at
// any given time.
	drwav_bool32 drwav_init_file(drwav* pWav, const char* filename);
	drwav_bool32 drwav_init_file_ex(drwav* pWav, const char* filename, drwav_chunk_proc onChunk, void* pChunkUserData, drwav_uint32 flags);

	// Helper for initializing a wave file for writing using stdio.
	//
	// This holds the internal FILE object until drwav_uninit() is called. Keep this in mind if you're caching drwav
	// objects because the operating system may restrict the number of file handles an application can have open at
	// any given time.
	drwav_bool32 drwav_init_file_write(drwav* pWav, const char* filename, const drwav_data_format* pFormat);
	drwav_bool32 drwav_init_file_write_sequential(drwav* pWav, const char* filename, const drwav_data_format* pFormat, drwav_uint64 totalSampleCount);

	// Helper for opening a wave file using stdio.
	//
	// This holds the internal FILE object until drwav_close() is called. Keep this in mind if you're caching drwav
	// objects because the operating system may restrict the number of file handles an application can have open at
	// any given time.
	drwav* drwav_open_file(const char* filename);
	drwav* drwav_open_file_ex(const char* filename, drwav_chunk_proc onChunk, void* pChunkUserData, drwav_uint32 flags);

	// Helper for opening a wave file for writing using stdio.
	//
	// This holds the internal FILE object until drwav_close() is called. Keep this in mind if you're caching drwav
	// objects because the operating system may restrict the number of file handles an application can have open at
	// any given time.
	drwav* drwav_open_file_write(const char* filename, const drwav_data_format* pFormat);
	drwav* drwav_open_file_write_sequential(const char* filename, const drwav_data_format* pFormat, drwav_uint64 totalSampleCount);

#endif  //DR_WAV_NO_STDIO

	// Helper for initializing a loader from a pre-allocated memory buffer.
	//
	// This does not create a copy of the data. It is up to the application to ensure the buffer remains valid for
	// the lifetime of the drwav object.
	//
	// The buffer should contain the contents of the entire wave file, not just the sample data.
	drwav_bool32 drwav_init_memory(drwav* pWav, const void* data, size_t dataSize);
	drwav_bool32 drwav_init_memory_ex(drwav* pWav, const void* data, size_t dataSize, drwav_chunk_proc onChunk, void* pChunkUserData, drwav_uint32 flags);

	// Helper for initializing a writer which outputs data to a memory buffer.
	//
	// dr_wav will manage the memory allocations, however it is up to the caller to free the data with drwav_free().
	//
	// The buffer will remain allocated even after drwav_uninit() is called. Indeed, the buffer should not be
	// considered valid until after drwav_uninit() has been called anyway.
	drwav_bool32 drwav_init_memory_write(drwav* pWav, void** ppData, size_t* pDataSize, const drwav_data_format* pFormat);
	drwav_bool32 drwav_init_memory_write_sequential(drwav* pWav, void** ppData, size_t* pDataSize, const drwav_data_format* pFormat, drwav_uint64 totalSampleCount);

	// Helper for opening a loader from a pre-allocated memory buffer.
	//
	// This does not create a copy of the data. It is up to the application to ensure the buffer remains valid for
	// the lifetime of the drwav object.
	//
	// The buffer should contain the contents of the entire wave file, not just the sample data.
	drwav* drwav_open_memory(const void* data, size_t dataSize);
	drwav* drwav_open_memory_ex(const void* data, size_t dataSize, drwav_chunk_proc onChunk, void* pChunkUserData, drwav_uint32 flags);

	// Helper for opening a writer which outputs data to a memory buffer.
	//
	// dr_wav will manage the memory allocations, however it is up to the caller to free the data with drwav_free().
	//
	// The buffer will remain allocated even after drwav_close() is called. Indeed, the buffer should not be
	// considered valid until after drwav_close() has been called anyway.
	drwav* drwav_open_memory_write(void** ppData, size_t* pDataSize, const drwav_data_format* pFormat);
	drwav* drwav_open_memory_write_sequential(void** ppData, size_t* pDataSize, const drwav_data_format* pFormat, drwav_uint64 totalSampleCount);


#ifndef DR_WAV_NO_CONVERSION_API
	// Opens and reads a wav file in a single operation.
	drwav_int16* drwav_open_and_read_pcm_frames_s16(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalFrameCount);
	float* drwav_open_and_read_pcm_frames_f32(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalFrameCount);
	drwav_int32* drwav_open_and_read_pcm_frames_s32(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalFrameCount);
#ifndef DR_WAV_NO_STDIO
	// Opens and decodes a wav file in a single operation.
	drwav_int16* drwav_open_file_and_read_pcm_frames_s16(const char* filename, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalFrameCount);
	float* drwav_open_file_and_read_pcm_frames_f32(const char* filename, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalFrameCount);
	drwav_int32* drwav_open_file_and_read_pcm_frames_s32(const char* filename, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalFrameCount);
#endif

	// Opens and decodes a wav file from a block of memory in a single operation.
	drwav_int16* drwav_open_memory_and_read_pcm_frames_s16(const void* data, size_t dataSize, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalFrameCount);
	float* drwav_open_memory_and_read_pcm_frames_f32(const void* data, size_t dataSize, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalFrameCount);
	drwav_int32* drwav_open_memory_and_read_pcm_frames_s32(const void* data, size_t dataSize, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalFrameCount);
#endif

	// Frees data that was allocated internally by dr_wav.
	void drwav_free(void* pDataReturnedByOpenAndRead);


	// DEPRECATED APIS
	// ===============
	drwav_uint64 drwav_read(drwav* pWav, drwav_uint64 samplesToRead, void* pBufferOut);
	drwav_uint64 drwav_read_s16(drwav* pWav, drwav_uint64 samplesToRead, drwav_int16* pBufferOut);
	drwav_uint64 drwav_read_f32(drwav* pWav, drwav_uint64 samplesToRead, float* pBufferOut);
	drwav_uint64 drwav_read_s32(drwav* pWav, drwav_uint64 samplesToRead, drwav_int32* pBufferOut);
	drwav_bool32 drwav_seek_to_sample(drwav* pWav, drwav_uint64 sample);
	drwav_uint64 drwav_write(drwav* pWav, drwav_uint64 samplesToWrite, const void* pData);
#ifndef DR_WAV_NO_CONVERSION_API
	drwav_int16* drwav_open_and_read_s16(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalSampleCount);
	float* drwav_open_and_read_f32(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalSampleCount);
	drwav_int32* drwav_open_and_read_s32(drwav_read_proc onRead, drwav_seek_proc onSeek, void* pUserData, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalSampleCount);
#ifndef DR_WAV_NO_STDIO
	drwav_int16* drwav_open_memory_and_read_s16(const void* data, size_t dataSize, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalSampleCount);
	float* drwav_open_file_and_read_f32(const char* filename, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalSampleCount);
	drwav_int32* drwav_open_file_and_read_s32(const char* filename, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalSampleCount);
#endif
	drwav_int16* drwav_open_memory_and_read_s16(const void* data, size_t dataSize, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalSampleCount);
	float* drwav_open_memory_and_read_f32(const void* data, size_t dataSize, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalSampleCount);
	drwav_int32* drwav_open_memory_and_read_s32(const void* data, size_t dataSize, unsigned int* channels, unsigned int* sampleRate, drwav_uint64* totalSampleCount);
#endif


#ifdef __cplusplus
}
#endif
#endif  // dr_wav_h
