#include "SoundEnvelope.h"

#include <QtMath>

#include <cmath>

namespace Sound
{
    namespace
    {
        int clampTimecents(int value)
        {
            return qBound(-12000, value, 8000);
        }

        double timecentsToSeconds(int timecents)
        {
            return std::pow(2.0, static_cast<double>(timecents) / 1200.0);
        }

        int secondsToTimecents(double seconds)
        {
            return clampTimecents(qRound(1200.0 * std::log2(qMax(0.001, seconds))));
        }
    } // namespace

    Sf2Envelope envelopeToSf2(const GbaEnvelope &envelope)
    {
        Sf2Envelope sf2;

        // M4A attack 0 is the extreme/fast end; 255 is the fastest step.
        const double attackSeconds = envelope.attack == 0
                ? 0.001
                : 255.0 / (static_cast<double>(envelope.attack) * 60.0);
        sf2.attackTimecents = secondsToTimecents(attackSeconds);

        // Decay/release are close to a 10 ms .. 4.25 s range.
        const double decaySeconds = 0.01 +
                (static_cast<double>(envelope.decay) / 255.0) * 4.24;
        const double releaseSeconds = 0.01 +
                (static_cast<double>(envelope.release) / 255.0) * 4.24;
        sf2.decayTimecents = secondsToTimecents(decaySeconds);
        sf2.releaseTimecents = secondsToTimecents(releaseSeconds);

        if (envelope.sustain == 0)
        {
            sf2.sustainCentibels = 1440;
        }
        else
        {
            const double level = static_cast<double>(envelope.sustain) / 255.0;
            sf2.sustainCentibels = qBound(
                        0, qRound(-200.0 * std::log10(level)), 1440);
        }
        return sf2;
    }

    GbaEnvelope envelopeFromSf2(const Sf2Envelope &sf2)
    {
        GbaEnvelope envelope;

        // Attack: 255 is the fastest. A very short SF2 attack maps to 255.
        if (sf2.attackTimecents <= -11999)
        {
            envelope.attack = 255;
        }
        else
        {
            const double seconds = timecentsToSeconds(sf2.attackTimecents);
            envelope.attack = static_cast<quint8>(
                        qBound(1, qRound(255.0 / (seconds * 60.0)), 255));
        }

        const double decaySeconds = timecentsToSeconds(sf2.decayTimecents);
        envelope.decay = static_cast<quint8>(
                    qBound(0, qRound((decaySeconds - 0.01) / 4.24 * 255.0), 255));

        if (sf2.sustainCentibels >= 1440)
        {
            envelope.sustain = 0;
        }
        else
        {
            const double level = std::pow(10.0, -sf2.sustainCentibels / 200.0);
            envelope.sustain = static_cast<quint8>(
                        qBound(0, qRound(level * 255.0), 255));
        }

        const double releaseSeconds = timecentsToSeconds(sf2.releaseTimecents);
        envelope.release = static_cast<quint8>(
                    qBound(0, qRound((releaseSeconds - 0.01) / 4.24 * 255.0), 255));

        return envelope;
    }

    bool envelopeMatchesSf2(const GbaEnvelope &gba, const Sf2Envelope &sf2)
    {
        const Sf2Envelope expected = envelopeToSf2(gba);
        return qAbs(expected.attackTimecents - sf2.attackTimecents) <= 100 &&
                qAbs(expected.decayTimecents - sf2.decayTimecents) <= 100 &&
                qAbs(expected.sustainCentibels - sf2.sustainCentibels) <= 20 &&
                qAbs(expected.releaseTimecents - sf2.releaseTimecents) <= 100;
    }
} // namespace Sound
