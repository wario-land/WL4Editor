#ifndef SOUNDENVELOPE_H
#define SOUNDENVELOPE_H

#include <QtGlobal>

namespace Sound
{
    struct GbaEnvelope
    {
        quint8 attack = 0;
        quint8 decay = 0;
        quint8 sustain = 255;
        quint8 release = 0;
    };

    struct Sf2Envelope
    {
        int attackTimecents = -12000;
        int decayTimecents = -12000;
        int sustainCentibels = 0;
        int releaseTimecents = -12000;
    };

    // Approximate conversion between the M4A 8-bit envelope fields and SF2
    // timecents/centibels. The M4A attack is "larger = faster"; decay and
    // release are "smaller = faster"; sustain is a 0..255 level.
    Sf2Envelope envelopeToSf2(const GbaEnvelope &envelope);
    GbaEnvelope envelopeFromSf2(const Sf2Envelope &envelope);
    bool envelopeMatchesSf2(const GbaEnvelope &gba, const Sf2Envelope &sf2);
} // namespace Sound

#endif // SOUNDENVELOPE_H
