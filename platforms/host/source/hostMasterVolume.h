#ifndef SOURCE_HOSTMASTERVOLUME_H_
#define SOURCE_HOSTMASTERVOLUME_H_

#ifdef __cplusplus
extern "C"
{
#endif

// Host-only stand-in for the hardware codec master volume.
// Sgtl5000Impl::setMasterVolume stores the gain; the SDL audio callback applies it.
void hostSetMasterVolumeGain(float volume);
float hostGetMasterVolumeGain(void);

#ifdef __cplusplus
}
#endif

#endif /* SOURCE_HOSTMASTERVOLUME_H_ */
