#pragma once

namespace ferb
{
/** Cambio de modo sin clics: baja una ganancia a 0, cambia la estructura del DSP, deja la
    cadena nueva calentarse en silencio y vuelve a subir.

        Open → FadingOut → SwitchPending → Holding → FadingIn → Open

    Se consulta una vez por trozo (de ≤ 256 muestras). Devuelve el modo con el que hay que
    procesar ese trozo y la ganancia al inicio y al final; el motor interpola por muestra.
    El modo solo cambia con la ganancia ya en 0. */
class ModeSwitch
{
public:
    void prepare (double sampleRate, double fadeSeconds, int holdSamples) noexcept
    {
        fadeSamples = static_cast<int> (sampleRate * fadeSeconds);
        hold = holdSamples;
    }

    void reset (int mode) noexcept
    {
        active = mode;
        state = State::Open;
        gain = 1.0f;
    }

    struct Step
    {
        int mode = 0;
        float gainStart = 1.0f;
        float gainEnd = 1.0f;
        bool switched = false; // primer trozo con el modo nuevo: reiniciar el DSP
    };

    Step update (int requested, int n) noexcept
    {
        Step step;
        step.gainStart = gain;

        if (state == State::Open && requested != active)
            state = State::FadingOut;

        if (state == State::SwitchPending)
        {
            active = requested;
            step.switched = true;
            holdLeft = hold;
            state = State::Holding;
            gain = 0.0f;
            step.gainStart = 0.0f;
        }

        step.mode = active;
        const float rate = fadeSamples > 0 ? 1.0f / static_cast<float> (fadeSamples) : 1.0f;

        switch (state)
        {
            case State::FadingOut:
                gain -= rate * static_cast<float> (n);
                if (gain <= 0.0f)
                {
                    gain = 0.0f;
                    state = State::SwitchPending;
                }
                break;

            case State::Holding:
                holdLeft -= n;
                if (holdLeft <= 0)
                    state = State::FadingIn;
                break;

            case State::FadingIn:
                gain += rate * static_cast<float> (n);
                if (gain >= 1.0f)
                {
                    gain = 1.0f;
                    state = State::Open;
                }
                break;

            case State::Open:
            case State::SwitchPending:
                break;
        }

        step.gainEnd = gain;
        return step;
    }

private:
    enum class State { Open, FadingOut, SwitchPending, Holding, FadingIn };

    State state = State::Open;
    int active = 0;
    int fadeSamples = 240, hold = 256, holdLeft = 0;
    float gain = 1.0f;
};
} // namespace ferb
