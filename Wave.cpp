#include "AudioData.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <exception>

static void checkStrings(std::ifstream &input, const char *test, int offset, const char *error_msg);

template <typename T>
static void checkData(std::ifstream &input, T test, int offset, const char *error_msg);

template <typename T>
static void writeSamples(T *data, std::ifstream &input, unsigned frame_count, unsigned channel_count, std::vector<float> &fdata);

struct Label
{
  char riff_label[4]; // (00) = {’R’,’I’,’F’,’F’} 
  unsigned riff_size; // (04) = 36 + data_size
  char file_tag[4];   // (08) = {’W’,’A’,’V’,’E’} 
};

struct Format 
{
  char fmt_label[4];                // (12) {’f’,’m’,’t’,’ ’} 
  unsigned fmt_size;                // (16) 16 
  unsigned short audio_format;      // (20) = 1 
  unsigned short channel_count;     // (22) = 1 or 2 
  unsigned int sampling_rate;       // (24) = 8000, 44100, etc. 
  unsigned int bytes_per_second;    // (28) = (see below) 
  unsigned short bytes_per_sample;  // (32) = (see below) 
  unsigned short bits_per_sample;   // (34) = 8 or 16 
};

AudioData::AudioData(const char * fname)
{
  std::ifstream input(fname, std::ios::binary);

  if (!input)
  {
    throw std::runtime_error("Could not open file");
  }

  // tests the riff section
  const char riff_test[] = "RIFF";
  checkStrings(input, riff_test, 0, "Not a RIFF");

  // tests the WAVE section
  const char wave_test[] = "WAVE";
  checkStrings(input, wave_test, 8, "Not a WAVE");

  // tests the fmt section
  const char fmt_test[] = "fmt ";
  checkStrings(input, fmt_test, 12, "fmt label incorrect");

  // checks the fmt size (should be 16)
  unsigned fmtSizeCheck = 16;
  checkData(input, fmtSizeCheck, 16, "fmt size incorrect");

  // checks the audio format (should be 1)
  unsigned short audioFormatTest = 1;
  checkData(input, audioFormatTest, 20, "Not an uncompressed format");

  // copies the channel count info to the wave data struct
  input.seekg(22);
  char *inputChar = new char[4];
  input.read(inputChar, sizeof(short));
  channel_count = *reinterpret_cast<short *>(inputChar);

  // copies the sampling rate info
  input.seekg(24);
  input.read(inputChar, sizeof(int));
  sampling_rate = *reinterpret_cast<int *>(inputChar);

  // checks the data label
  const char data_label_test[] = "data";
  checkStrings(input, data_label_test, 36, "data label incorrect");

  // finds the amount of data contained in the samples
  input.seekg(40);
  input.read(inputChar, sizeof(unsigned));
  unsigned size = *reinterpret_cast<unsigned *>(inputChar);

  input.seekg(34);
  input.read(inputChar, sizeof(short));
  unsigned bits_per_sample = *reinterpret_cast<short *>(inputChar);

  /* decides to record the frame count for mono or stereo audio
   * frame count for mono is just the amount of samples
   * frame count for stereo is the amount of left/right pairs of samples */
  frame_count = size / (channel_count * (bits_per_sample / 8));

  /* dynamically allocates the amount of data needed to hold all the samples
   * then copies all the sample data into our wave data struct               */
  fdata.resize(frame_count * channel_count);
  input.seekg(44);
  
  if (bits_per_sample == 8)
  {
    unsigned char data;
    writeSamples(&data, input, frame_count, channel_count, fdata);
  }
  else if (bits_per_sample == 16)
  {
    short data;
    writeSamples(&data, input, frame_count, channel_count, fdata);
  }

  delete[] inputChar;
  input.close();
}

bool waveWrite(const char * fname, const AudioData & ad, unsigned bits)
{
  FILE *fp = fopen(fname, "wb");

  if (!fp)
  {
    //throw std::runtime_error("could not open the file");
    return false;
  }

  // creates a wave header sample
  const char riff_label[] = "RIFF";
  const unsigned riff_size = 36 + ((ad.frames() * ad.channels()) * (bits / 8));
  const char file_tag[] = "WAVE";
  const char fmt_label[] = "fmt ";
  const unsigned fmt_size = 16;
  const unsigned short audio_format = 1;
  const unsigned short channel_count = ad.channels();
  const unsigned sample_rate = ad.rate();
  const unsigned bytes_per_second = ad.rate() * ((bits / 8) * channel_count);
  const unsigned short bytes_per_sample = (bits / 8) * channel_count;
  const unsigned short bits_per_sample = bits;
  const char data_label[] = "data";
  const unsigned data_size = ad.frames() * ad.channels() * (bits / 8);

  // writes all the wave header data to a new wave file
  fwrite(&riff_label, 1, sizeof(riff_label) - 1, fp);
  fwrite(&riff_size, 1, sizeof(unsigned), fp);
  fwrite(&file_tag, 1, sizeof(file_tag) - 1, fp);
  fwrite(&fmt_label, 1, sizeof(fmt_label) - 1, fp);
  fwrite(&fmt_size, 1, sizeof(unsigned), fp);
  fwrite(&audio_format, 1, sizeof(unsigned short), fp);
  fwrite(&channel_count, 1, sizeof(unsigned short), fp);
  fwrite(&sample_rate, 1, sizeof(unsigned), fp);
  fwrite(&bytes_per_second, 1, sizeof(unsigned), fp);
  fwrite(&bytes_per_sample, 1, sizeof(unsigned short), fp);
  fwrite(&bits_per_sample, 1, sizeof(unsigned short), fp);
  fwrite(&data_label, 1, sizeof(data_label) - 1, fp);
  fwrite(&data_size, 1, sizeof(unsigned), fp);

  // writes all the samples to the wave file
  if (bits == 8)
  {
    for (unsigned i = 0; i < (ad.frames() * channel_count); i++)
    {
      float data = ad.data()[i];
      int voltage = int(ad.data()[i] * 127);
      unsigned char sample = voltage + 128;
      fwrite(&sample, 1, sizeof(char), fp);
    }
  }
  else if (bits == 16)
  {
    for (unsigned i = 0; i < (ad.frames() * channel_count); i++)
    {
      short voltage = short(ad.data()[i] * 32767);
      fwrite(&voltage, 1, sizeof(short), fp);
    }
  }

  fclose(fp);

  return true;
}

static void checkStrings(std::ifstream &input, const char * test, int offset, const char * error_msg)
{
  input.seekg(offset);
  char inputString[5]; /* The 4 characters plus the NULL terminator */
  input.read(inputString, 4);
  inputString[4] = 0; // NULL terminator

  if (strcmp(test, inputString))
  {
    input.close();
    throw std::runtime_error(error_msg);
  }
}

template <typename T>
static void checkData(std::ifstream &input, T test, int offset, const char * error_msg)
{
  input.seekg(offset);
  char *inputChar = new char[sizeof(T)];
  input.read(inputChar, sizeof(T));
  T inputValue = *reinterpret_cast<T *>(inputChar);
  delete inputChar;
  if (inputValue != test)
  {
    input.close();
    throw std::runtime_error(error_msg);
  }
}

template<typename T>
void writeSamples(T *data, std::ifstream &input, unsigned frame_count, unsigned channel_count, std::vector<float> &fdata)
{
  data = new T[frame_count * channel_count];
  input.read(reinterpret_cast<char *>(data), (frame_count * channel_count * sizeof(T)));

  if (sizeof(T) == 1) // Bit depth is 8 bit
  {
    for (unsigned i = 0; i < frame_count * channel_count; i++)
    {
      float voltage = float(data[i]) - 128;
      fdata[i] = voltage / 128.0f;
    }
  }
  else if (sizeof(T) == 2) // Bit depth is 16 bit
  {
    for (unsigned i = 0; i < frame_count * channel_count; i++)
    {
      fdata[i] = float(data[i]) / 32767.0f;
    }
  }
  delete[] data;
}

AudioData::AudioData(unsigned nframes, unsigned R, unsigned nchannels) :
  frame_count(nframes), sampling_rate(R), channel_count(nchannels)
{
  fdata.resize(nframes * nchannels);
}

float AudioData::sample(unsigned frame, unsigned channel) const
{
  return fdata[(2 * frame) + channel];
}

float & AudioData::sample(unsigned frame, unsigned channel)
{
  return fdata[(channel_count * frame) + channel];
}

static float calculateDCOffset(AudioData &ad, int channel)
{
  float dcOffset = 0;

  for (unsigned i = 0; i < ad.frames(); i++)
  {
    dcOffset += ad.data()[(i * ad.channels()) + channel];
  }
  dcOffset /= ad.frames();

  return dcOffset;
}

static float calculateLargestSample(AudioData &ad)
{
  float largestSample = 0;

  for (unsigned i = 0; i < ad.frames(); i++)
  {
    for (unsigned j = 0; j < ad.channels(); j++)
    {
      if (abs(ad.data()[(ad.channels() * i) + j]) > largestSample)
      {
        largestSample = abs(ad.data()[(ad.channels() * i) + j]);
      }
    }
  }
  return largestSample;
}

static void removeDCOffset(AudioData &ad, float offset, int channel)
{
  for (unsigned i = 0; i < ad.frames(); i++)
  {
    ad.data()[(ad.channels() * i) + channel] -= offset;
  }
}

static void changeAmplitude(AudioData &ad, float dB, float largestSample)
{
  float ceiling = 1.0f * float(pow(10, dB / 20));
  ceiling /= largestSample;

  for (unsigned i = 0; i < ad.frames(); i++)
  {
    for (unsigned j = 0; j < ad.channels(); j++)
    {
      ad.data()[(ad.channels() * i) + j] *= ceiling;
    }
  }
}

void normalize(AudioData & ad, float dB)
{
  std::vector<float> dcOffsets;
  float largestSample;

  dcOffsets.resize(ad.channels());

  for (unsigned i = 0; i < ad.channels(); i++)
  {
    dcOffsets[i] = calculateDCOffset(ad, i);
  }

  for (unsigned i = 0; i < ad.channels(); i++)
  {
    removeDCOffset(ad, dcOffsets[i], i);
  }

  largestSample = calculateLargestSample(ad);
  changeAmplitude(ad, dB, largestSample);
}