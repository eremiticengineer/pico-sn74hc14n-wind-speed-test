#pragma once

#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"

// 1 turn/sec = 1.492 mph
//
// Store wind speed internally as tenths of mph:
//   75  = 7.5 mph
//   149 = 14.9 mph
static constexpr int32_t WIND_SCALE_NUM = 14920;
static constexpr int32_t WIND_SCALE_DEN = 10000;

class WindMonitor
{
public:
    // Running average contains the last 10 one-second samples.
    static constexpr int RUN_AVG_SIZE = 10;

    // One maximum gust value per minute for the previous hour.
    static constexpr int GUST_MIN_BUF = 60;

    WindMonitor()
        : clicks_1s_(0),
          clicks_gust_(0),
          run_sum_(0),
          run_idx_(0),
          run_filled_(0),
          gust_minute_idx_(0)
    {
        for (int i = 0; i < RUN_AVG_SIZE; ++i)
        {
            run_buf_[i] = 0;
        }

        for (int i = 0; i < GUST_MIN_BUF; ++i)
        {
            gust_min_buf_[i] = 0;
        }
    }

    // -------------------------------------------------------------------------
    // ISR
    // -------------------------------------------------------------------------

    // Call this for every anemometer pulse.
    inline void onPulse()
    {
        clicks_1s_++;
        clicks_gust_++;
    }

    // -------------------------------------------------------------------------
    // Sampling
    // -------------------------------------------------------------------------

    // Called once per second.
    //
    // Returns wind speed in tenths of mph:
    //   75 = 7.5 mph
    inline int32_t sampleAverage1s()
    {
        const uint32_t clicks = swapAndClear_(clicks_1s_);

        const int32_t mph10 = toMph10(clicks, 1000);

        run_sum_ -= run_buf_[run_idx_];

        run_buf_[run_idx_] = mph10;

        run_sum_ += mph10;

        run_idx_ = (run_idx_ + 1) % RUN_AVG_SIZE;

        if (run_filled_ < RUN_AVG_SIZE)
        {
            run_filled_++;
        }

        return mph10;
    }

    // Called at the end of the chosen gust interval.
    //
    // A short interval such as 200 ms produces very coarse results:
    //
    //   1 pulse in 200 ms = 7.5 mph
    //   2 pulses           = 14.9 mph
    //
    // A 2-second interval gives much better resolution:
    //
    //   1 pulse in 2 sec = 0.7 mph
    //   2 pulses         = 1.5 mph
    //
    // Returns wind speed in tenths of mph.
    inline int32_t sampleGustInterval(uint32_t interval_ms)
    {
        const uint32_t clicks = swapAndClear_(clicks_gust_);

        const int32_t mph10 = toMph10(clicks, interval_ms);

        if (mph10 > gust_min_buf_[gust_minute_idx_])
        {
            gust_min_buf_[gust_minute_idx_] = mph10;
        }

        return mph10;
    }

    // Advance to the next minute in the one-hour gust history.
    inline void rotateMinute()
    {
        gust_minute_idx_ =
            (gust_minute_idx_ + 1) % GUST_MIN_BUF;

        gust_min_buf_[gust_minute_idx_] = 0;
    }

    // -------------------------------------------------------------------------
    // Getters
    // -------------------------------------------------------------------------

    // Running average in tenths of mph.
    inline int32_t getRunningAverageMph10() const
    {
        if (run_filled_ == 0)
        {
            return 0;
        }

        return static_cast<int32_t>(
            run_sum_ / run_filled_
        );
    }

    // Maximum gust during the previous hour, in tenths of mph.
    inline int32_t getHourlyMaxGustMph10() const
    {
        int32_t max_mph10 = 0;

        for (int i = 0; i < GUST_MIN_BUF; ++i)
        {
            if (gust_min_buf_[i] > max_mph10)
            {
                max_mph10 = gust_min_buf_[i];
            }
        }

        return max_mph10;
    }

    // -------------------------------------------------------------------------
    // Conversion
    // -------------------------------------------------------------------------

    // Convert a pulse count over an arbitrary interval into tenths of mph.
    //
    // Example:
    //
    //   1 pulse over 1000 ms
    //
    //   = 1 revolution/sec
    //   = 1.492 mph
    //   = 14.92 tenths
    //   = 15 after rounding
    //
    static inline int32_t toMph10(
        uint32_t clicks,
        uint32_t interval_ms)
    {
        if (interval_ms == 0)
        {
            return 0;
        }

        const int64_t numerator =
            static_cast<int64_t>(WIND_SCALE_NUM) *
            clicks *
            1000LL *
            10LL;

        const int64_t denominator =
            static_cast<int64_t>(WIND_SCALE_DEN) *
            interval_ms;

        // Round rather than truncate.
        return static_cast<int32_t>(
            (numerator + denominator / 2) /
            denominator
        );
    }

private:
    // Atomically obtain and reset a counter shared between ISR and task.
    inline uint32_t swapAndClear_(
        volatile uint32_t& counter)
    {
        taskENTER_CRITICAL();

        const uint32_t value = counter;

        counter = 0;

        taskEXIT_CRITICAL();

        return value;
    }

    // ISR counters
    volatile uint32_t clicks_1s_;
    volatile uint32_t clicks_gust_;

    // 10-second running average
    int32_t run_buf_[RUN_AVG_SIZE];
    int64_t run_sum_;
    int run_idx_;
    int run_filled_;

    // One maximum gust per minute, covering the previous hour
    int32_t gust_min_buf_[GUST_MIN_BUF];
    int gust_minute_idx_;
};