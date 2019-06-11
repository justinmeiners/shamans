
#include "snd.h"
#include <assert.h>


#if SND_OGG_VORBIS
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#endif

#if SND_OGG_VORBIS
int _Snd_FromOGG(Snd* snd, FILE* file)
{
    OggVorbis_File vf;
    
    if (ov_open_callbacks(file, &vf, NULL, 0, OV_CALLBACKS_DEFAULT) < 0)
    {
        return NULL;
    }
    
    Snd* snd = Snd_Create();
    
    vorbis_info* info = ov_info(&vf,-1);
    snd->sampleRate = (int)info->rate;
    Snd_Rebuild(snd, ov_pcmotal(&vf, -1), info->channels, 16);
    
    int eof = 0;
    int current_section;
    
    size_t offset = 0;

    while (!eof)
    {
        long ret = ov_read(&vf, snd->_data.p + offset, Snd_SampleBufferLength(snd), 0, 2, 1, &current_section);
        
        if (ret == 0)
        {
            eof = 1;
        }
        else if (ret < 0)
        {
            goto error;
        }
        else
        {
            offset += ret;
        }
    }
    
    ov_clear(&vf);

    return snd;

error:
    Snd_Destroy(snd);
    ov_clear(&vf);
    return NULL;
}
#endif

