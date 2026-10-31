#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace nel
{
/** Etapa de picos (A) de un canal: ganancia suavizada que garantiza el techo con anticipación.

    1. `required` = ganancia necesaria para que la muestra no pase del techo (≤ 1).
    2. Mínimo móvil de las últimas `window` ganancias (cola monótona, coste constante por muestra).
    3. Dos medias móviles en cascada de longitudes a = window/2 + 1 y b = window − a + 1, que juntas
       abarcan `window` muestras: los pesos suman 1 y cada valor de la ventana es ≤ la ganancia
       requerida de la muestra a la que se aplica, así la ganancia suavizada nunca supera la requerida.
    4. El audio se retrasa `window − 1` muestras (lo hace quien llama) y se multiplica por el resultado. */
class PeakStage
{
public:
    void prepare (int maxWindow)
    {
        size_t cap = 4;
        while (cap < static_cast<size_t> (maxWindow) + 2)
            cap <<= 1;
        values.assign (cap, 1.0f);
        indices.assign (cap, 0);
        mask = cap - 1;
        ringA.assign (static_cast<size_t> (maxWindow) + 2, 1.0f);
        ringB.assign (static_cast<size_t> (maxWindow) + 2, 1.0f);
        configure (1);
    }

    /** Fija la ventana y reinicia el estado (ganancia 1). No asigna memoria. */
    void configure (int newWindow) noexcept
    {
        window = std::max (1, newWindow);
        lenA = window / 2 + 1;
        lenB = window - lenA + 1;
        head = tail = 0;
        counter = 0;
        std::fill_n (ringA.begin(), lenA, 1.0f);
        std::fill_n (ringB.begin(), lenB, 1.0f);
        posA = posB = 0;
        sumA = lenA;
        sumB = lenB;
    }

    int getWindow() const noexcept { return window; }

    float process (float required) noexcept
    {
        // Cola monótona: los valores crecen de la cabeza a la cola.
        while (tail != head && values[(tail - 1) & mask] >= required)
            --tail;
        values[tail & mask] = required;
        indices[tail & mask] = counter;
        ++tail;
        while (indices[head & mask] <= counter - window)
            ++head;
        const float minimum = values[head & mask];
        ++counter;

        sumA += static_cast<double> (minimum) - static_cast<double> (ringA[posA]);
        ringA[posA] = minimum;
        posA = posA + 1 == static_cast<size_t> (lenA) ? 0 : posA + 1;
        const float mid = static_cast<float> (sumA / lenA);

        sumB += static_cast<double> (mid) - static_cast<double> (ringB[posB]);
        ringB[posB] = mid;
        posB = posB + 1 == static_cast<size_t> (lenB) ? 0 : posB + 1;
        return static_cast<float> (sumB / lenB);
    }

private:
    std::vector<float> values, ringA, ringB;
    std::vector<std::int64_t> indices;
    size_t mask = 0, head = 0, tail = 0, posA = 0, posB = 0;
    std::int64_t counter = 0;
    int window = 1, lenA = 1, lenB = 1;
    double sumA = 1.0, sumB = 1.0;
};
} // namespace nel
