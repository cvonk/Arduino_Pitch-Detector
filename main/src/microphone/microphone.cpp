/** 
 * @brief Asynchronous microphone driver
 * 
 * © Copyright 2015-2016,2022 Johan Vonk
 * 
 * This file is part of Arduino_pitch-detector.
 * 
 * Arduino_pitch-detector is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 * 
 * Arduino_pitch-detector is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with Arduino_pitch-detector. If not, see <https://www.gnu.org/licenses/>.
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: Copyright 2015-2016,2022 Johan Vonk
 **/

#include <Arduino.h>
#include <stdint.h>
#include <limits.h>

#include "../../config.h"
#include "../../sample_t.h"
#include "../debug/debug.h"
#include "microphone.h"
#include "adc_t.h"

typedef struct amplitudeRange_t {
    sample_t min;
    sample_t max;
} amplitudeRange_t;

typedef struct microphone_t {
    sampleCnt_t cnt;
    amplitudeRange_t range;
    sample_t * samples;
    uint8_t analogPort;
    uint8_t prescaler;
} microphone_t;

microphone_t volatile _my = {};  // data updates in ISR

void
microphone_begin(uint8_t const port)
{
    _my.analogPort = port;

    // allocate memory (no freed, 'cause `loop` never stops)
    _my.samples = new sample_t[CONFIG_MIDIMIKE_WINDOW_SIZE];
    ASSERT((_my.samples));

    // determine prescaler setting for ADCSRA
    _my.prescaler = 0;
    for (uint32_t d = 16e6 / 13; CONFIG_MIDIMIKE_SAMPLE_RATE < d; d /= 2) {
        _my.prescaler++;
    }
    
    microphone_start();  // start gathering samples
}

void
microphone_start(void)
{
    // init private date for ISR (so we don't have to test for ii==0 inside the ISR)
    microphone_t volatile * const my = &_my;
    my->cnt = 0;

    // start gathering new samples (no need to disable interrupts, after all this interrupt is off, and we're turning it on here)

    ADCSRB =
        ADCSRB_MULTIPLEXER_ENABLE;

    ADMUX =
        ADMUX_REFERENCE_EXT |
        ADMUX_LEFT_ALIGN |
        (ADMUX_INPUT_MASK & my->analogPort);

    ADCSRA =
        my->prescaler |
        ADCSRA_AUTO_TRIGGER_ENABLE |
        ADCSRA_IRQ_ENABLE |
        ADCSRA_CONVERT_ENABLE |
        ADCSRA_CONVERT_START;
}


samples_t  // returns pointer to array of data samples, NULL on failure
microphone_get_samples(amplitude_t * const amplitudePtr)
{
    while (ADCSRA & ADCSRA_IRQ_ENABLE) {
        // spin wait until all samples are available
    }

    microphone_t volatile * const my = &_my;

    bool clipping = (my->range.max == SCHAR_MIN) || (my->range.max == SCHAR_MAX);
    amplitude_t amplitude = (int16_t)my->range.max - my->range.min; // top-top [0..255]

    *amplitudePtr = amplitude/2; // range [0..127]

    return (amplitude / 2 > CONFIG_MIDIMIKE_AUDIBLE_THRESHOLD) && !clipping ? my->samples : NULL;
}

// interrupt service routine
ISR (ADC_vect)
{
    microphone_t volatile * const my = &_my;
    sample_t const s = ADCH + SCHAR_MIN;  // remove voltage bias by changing range from [0..255] to [-128..127]

        // update min and max, so peak-to-peak value can be determined
    if (my->cnt == 0) {
        my->range.min = SCHAR_MAX;
        my->range.max = SCHAR_MIN;
    }
    if (s < my->range.min) {
        my->range.min = s;
    }
    if (s > my->range.max) {
        my->range.max = s;
    }

    if (my->cnt < CONFIG_MIDIMIKE_WINDOW_SIZE) {
        my->samples[my->cnt++] = s;
    } else {            
        ADCSRA =  // we're done
            my->prescaler |
            ADCSRA_AUTO_TRIGGER_DISABLE |
            ADCSRA_IRQ_DISABLE |
            ADCSRA_CONVERT_ENABLE;
    }
}
