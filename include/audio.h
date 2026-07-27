#ifndef AUDIO_H
#define AUDIO_H

// Camada fina de áudio, para o jogo compilar tanto no Windows quanto no Linux.
//
//   audioInit()      -> uma vez, antes de tocar qualquer som
//   audioPlay(path)  -> toca um .wav sem travar o jogo (assíncrono)
//   audioShutdown()  -> libera os recursos ao sair
//
// No Windows a implementação é o PlaySound do MMSystem (o mesmo de antes).
// No Linux é OpenAL: o .wav é lido e enviado pra placa de som na primeira vez
// que é tocado, e nas próximas só reinicia o buffer já carregado.
// Em ambos os casos, tocar um som que já está tocando reinicia ele do começo.
//
// Compilando com -DAUDIO_DISABLED, tudo vira função vazia e o jogo roda mudo
// (útil em máquina sem OpenAL instalado).

#include <string>

inline bool audioInit();
inline void audioPlay(const char *filename);
inline void audioShutdown();

// ---------------------------------------------------------------------------
#if defined(AUDIO_DISABLED)
// ---------------------------------------------------------------------------

inline bool audioInit() { return false; }
inline void audioPlay(const char *) {}
inline void audioShutdown() {}

// ---------------------------------------------------------------------------
#elif defined(_WIN32)
// ---------------------------------------------------------------------------

#include <windows.h>
#include <mmsystem.h>

inline bool audioInit() { return true; }

inline void audioPlay(const char *filename)
{
    PlaySound(filename, NULL, SND_ASYNC | SND_FILENAME);
}

inline void audioShutdown()
{
    PlaySound(NULL, NULL, 0); // Interrompe qualquer som ainda tocando
}

// ---------------------------------------------------------------------------
#else // Linux / OpenAL
// ---------------------------------------------------------------------------

#include <AL/al.h>
#include <AL/alc.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

// Um som carregado: o buffer com as amostras e a fonte que o reproduz.
struct AudioClip
{
    ALuint buffer;
    ALuint source;
};

// Estado global do sistema de áudio. Em funções (e não como variáveis soltas)
// pra continuar tudo em header, sem precisar de um .cpp separado.
inline ALCdevice *&audioDevice()
{
    static ALCdevice *device = nullptr;
    return device;
}

inline ALCcontext *&audioContext()
{
    static ALCcontext *context = nullptr;
    return context;
}

// Sons já carregados, indexados pelo caminho do arquivo. Um .wav que falhou ao
// carregar fica no mapa com buffer 0, pra não tentar reler o arquivo do disco a
// cada vez que o jogo pedir pra tocá-lo.
inline std::map<std::string, AudioClip> &audioCache()
{
    static std::map<std::string, AudioClip> cache;
    return cache;
}

// Lê um .wav PCM (8 ou 16 bits, mono ou estéreo) para a memória.
//
// O cabeçalho não tem tamanho fixo: um RIFF é uma sequência de blocos
// (id + tamanho + conteúdo), e os arquivos exportados por editores costumam
// trazer blocos extras como "LIST" antes do áudio de fato. Por isso os blocos
// são percorridos um a um até achar o "fmt " e o "data", em vez de pular um
// número fixo de bytes.
inline bool carregarWAV(const std::string &filename,
                        std::vector<char> &samples,
                        ALenum &format,
                        ALsizei &sampleRate)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Erro ao abrir o arquivo WAV: " << filename << std::endl;
        return false;
    }

    std::vector<char> bytes((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    file.close();

    // Cabeçalho RIFF: "RIFF" <tamanho> "WAVE"
    if (bytes.size() < 12 ||
        std::memcmp(&bytes[0], "RIFF", 4) != 0 ||
        std::memcmp(&bytes[8], "WAVE", 4) != 0)
    {
        std::cerr << "Arquivo nao e um WAV valido: " << filename << std::endl;
        return false;
    }

    uint16_t canais = 0, bitsPorAmostra = 0, formatoPCM = 0;
    uint32_t taxaAmostragem = 0;
    bool achouFmt = false, achouData = false;

    size_t pos = 12;
    while (pos + 8 <= bytes.size())
    {
        char id[5] = {0};
        std::memcpy(id, &bytes[pos], 4);
        uint32_t tamanho;
        std::memcpy(&tamanho, &bytes[pos + 4], 4);
        size_t conteudo = pos + 8;

        if (conteudo + tamanho > bytes.size())
            tamanho = (uint32_t)(bytes.size() - conteudo); // Arquivo truncado

        if (std::memcmp(id, "fmt ", 4) == 0 && tamanho >= 16)
        {
            std::memcpy(&formatoPCM, &bytes[conteudo + 0], 2);
            std::memcpy(&canais, &bytes[conteudo + 2], 2);
            std::memcpy(&taxaAmostragem, &bytes[conteudo + 4], 4);
            std::memcpy(&bitsPorAmostra, &bytes[conteudo + 14], 2);
            achouFmt = true;
        }
        else if (std::memcmp(id, "data", 4) == 0)
        {
            samples.assign(bytes.begin() + conteudo,
                           bytes.begin() + conteudo + tamanho);
            achouData = true;
        }

        // Os blocos são alinhados em 2 bytes: um de tamanho ímpar tem 1 byte
        // de preenchimento que não entra na contagem.
        pos = conteudo + tamanho + (tamanho % 2);
    }

    if (!achouFmt || !achouData)
    {
        std::cerr << "WAV sem os blocos fmt/data: " << filename << std::endl;
        return false;
    }
    if (formatoPCM != 1)
    {
        std::cerr << "WAV comprimido nao suportado (use PCM): " << filename << std::endl;
        return false;
    }

    if (canais == 1)
        format = (bitsPorAmostra == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
    else if (canais == 2)
        format = (bitsPorAmostra == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
    else
    {
        std::cerr << "WAV com " << canais << " canais nao suportado: " << filename << std::endl;
        return false;
    }

    if (bitsPorAmostra != 8 && bitsPorAmostra != 16)
    {
        std::cerr << "WAV de " << bitsPorAmostra << " bits nao suportado: " << filename << std::endl;
        return false;
    }

    sampleRate = (ALsizei)taxaAmostragem;
    return true;
}

inline bool audioInit()
{
    if (audioContext() != nullptr)
        return true;

    audioDevice() = alcOpenDevice(nullptr); // Placa de som padrão do sistema
    if (!audioDevice())
    {
        std::cerr << "Nenhum dispositivo de audio encontrado, o jogo vai rodar mudo." << std::endl;
        return false;
    }

    audioContext() = alcCreateContext(audioDevice(), nullptr);
    if (!audioContext() || !alcMakeContextCurrent(audioContext()))
    {
        std::cerr << "Falha ao iniciar o contexto de audio, o jogo vai rodar mudo." << std::endl;
        if (audioContext())
            alcDestroyContext(audioContext());
        alcCloseDevice(audioDevice());
        audioContext() = nullptr;
        audioDevice() = nullptr;
        return false;
    }
    return true;
}

inline void audioPlay(const char *filename)
{
    if (audioContext() == nullptr)
        return; // Sem áudio disponível: o jogo segue normalmente, só mudo

    std::string caminho(filename);
    auto it = audioCache().find(caminho);

    if (it == audioCache().end())
    {
        AudioClip clip = {0, 0};
        std::vector<char> samples;
        ALenum format;
        ALsizei sampleRate;

        if (carregarWAV(caminho, samples, format, sampleRate))
        {
            alGenBuffers(1, &clip.buffer);
            alBufferData(clip.buffer, format, samples.data(),
                         (ALsizei)samples.size(), sampleRate);
            alGenSources(1, &clip.source);
            alSourcei(clip.source, AL_BUFFER, (ALint)clip.buffer);
            // Som 2D: toca sempre no mesmo volume, sem depender da posição
            // do ouvinte na cena.
            alSourcei(clip.source, AL_SOURCE_RELATIVE, AL_TRUE);
        }
        it = audioCache().insert({caminho, clip}).first;
    }

    if (it->second.buffer == 0)
        return; // Arquivo com problema, já reportado no carregamento

    // Reinicia do começo se ainda estiver tocando (mesmo comportamento do
    // PlaySound no Windows).
    alSourceStop(it->second.source);
    alSourcePlay(it->second.source);
}

inline void audioShutdown()
{
    if (audioContext() == nullptr)
        return;

    for (auto &item : audioCache())
    {
        if (item.second.buffer != 0)
        {
            alSourceStop(item.second.source);
            alDeleteSources(1, &item.second.source);
            alDeleteBuffers(1, &item.second.buffer);
        }
    }
    audioCache().clear();

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(audioContext());
    alcCloseDevice(audioDevice());
    audioContext() = nullptr;
    audioDevice() = nullptr;
}

#endif // Plataforma

#endif // AUDIO_H
