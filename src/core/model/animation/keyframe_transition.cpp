#include "keyframe_transition.hpp"
#include "math/bezier/segment.hpp"

namespace {

constexpr QPointF bound_vec(const QPointF& v)
{
    return {
        qBound(math::scalar_type<QPointF>(0), v.x(), math::scalar_type<QPointF>(1)),
        qBound(math::scalar_type<QPointF>(0), v.y(), math::scalar_type<QPointF>(1))
    };
}

} // namespace

model::KeyframeTransition::Descriptive model::KeyframeTransition::before_descriptive() const
{
    if ( hold_ )
        return Hold;

    if ( bezier_.points()[1] == bezier_.points()[0] )
        return Linear;

    if ( bezier_.points()[1].y() == 0 )
        return Ease;

    return Custom;
}

model::KeyframeTransition::Descriptive model::KeyframeTransition::after_descriptive() const
{
    if ( hold_ )
        return Hold;

    if ( bezier_.points()[2] == bezier_.points()[3] )
        return Linear;

    if ( bezier_.points()[2].y() == 1 )
        return Ease;

    return Custom;
}

void model::KeyframeTransition::set_before_descriptive(model::KeyframeTransition::Descriptive d)
{
    switch ( d )
    {
        case Hold:
            set_hold(true);
            return;
        case Linear:
            bezier_.points()[1] = bezier_.points()[0];
            hold_ = false;
            break;
        case Ease:
            bezier_.points()[1] = QPointF{1./3., 0};
            hold_ = false;
            break;
        case Custom:
            hold_ = false;
            break;
    }
}

void model::KeyframeTransition::set_after_descriptive(model::KeyframeTransition::Descriptive d)
{
    switch ( d )
    {
        case Hold:
            set_hold(true);
            return;
        case Linear:
            bezier_.points()[2] = bezier_.points()[3];
            hold_ = false;
            break;
        case Ease:
            bezier_.points()[2] = QPointF{2./3., 1};
            hold_ = false;
            break;
        case Custom:
            hold_ = false;
            break;
    }
}

void model::KeyframeTransition::set_after(const QPointF& after)
{
    sample_cache_.clear();
    bezier_.points()[2] = bound_vec(after);
}

void model::KeyframeTransition::set_before(const QPointF& before)
{
    sample_cache_.clear();
    bezier_.points()[1] = bound_vec(before);
}

void model::KeyframeTransition::set_handles(const QPointF& before, const QPointF& after)
{
    set_before(before);
    set_after(after);
}

void model::KeyframeTransition::set_hold(bool hold)
{
    hold_ = hold;
}

namespace {

static constexpr const int NEWTON_ITERATIONS = 6;
static constexpr const double NEWTON_MIN_SLOPE = 0.001;
static constexpr const double SUBDIVISION_PRECISION = 0.0000001;
static constexpr const int SUBDIVISION_MAX_ITERATIONS = 10;
static constexpr const int SPLINE_TABLE_SIZE = 11;
static constexpr const double SAMPLE_STEP_SIZE = 1.0 / (SPLINE_TABLE_SIZE - 1.0);

using Bez = math::bezier::CubicBezierSolver<QPointF>;

double _binary_subdivide(double x, double interval_start, double interval_end, const Bez& bez)
{
    double current_x = 0;
    double t = 0;

    for ( int i = 0; i < SUBDIVISION_MAX_ITERATIONS; i++)
    {
        if ( i != 0 && std::abs(current_x) < SUBDIVISION_PRECISION )
            break;
        t = (interval_start + interval_end) / 2.0;
        current_x = bez.solve_component(t, 0) - x;
        if ( current_x > 0.0 )
            interval_end = t;
        else
            interval_start = t;
    }
    return t;
}

double _newton_raphson(double x, double t_guess, const Bez& bez)
{
    for ( int i = 0; i < NEWTON_ITERATIONS; i++)
    {
        double slope = bez.derivative(t_guess, 0);
        if ( slope == 0 )
            return t_guess;
        double current_x = bez.solve_component(t_guess, 0) - x;
        t_guess -= current_x / slope;
    }
    return t_guess;
}

void _get_sample_values(const Bez& bez, model::detail::SampleCache& sample_cache)
{
    if ( sample_cache.empty() )
    {
        sample_cache.resize(SPLINE_TABLE_SIZE);
        for ( int i = 0; i < SPLINE_TABLE_SIZE; i++ )
            sample_cache[i] = bez.solve_component(i *  SAMPLE_STEP_SIZE, 0);
    }
}

double t_for_x(double x, const Bez& bez, model::detail::SampleCache& sample_cache)
{
    _get_sample_values(bez, sample_cache);
    double interval_start = 0;
    int current_sample = 1;
    int last_sample = SPLINE_TABLE_SIZE - 1;
    while ( current_sample != last_sample && sample_cache[current_sample] <= x )
    {
        interval_start += SAMPLE_STEP_SIZE;
        current_sample += 1;
    }
    current_sample -= 1;

    double dist = (x - sample_cache[current_sample]) / (sample_cache[current_sample+1] - sample_cache[current_sample]);
    double t_guess = interval_start + dist * SAMPLE_STEP_SIZE;
    double initial_slope = bez.derivative(t_guess, 0);
    if ( initial_slope >= NEWTON_MIN_SLOPE )
        return _newton_raphson(x, t_guess, bez);
    if ( initial_slope == 0 )
        return t_guess;
    return _binary_subdivide(x, interval_start, interval_start + SAMPLE_STEP_SIZE, bez);
}


} // namespace

double model::KeyframeTransition::lerp_factor(double ratio) const
{
    if ( ratio <= 0 || hold_ )
        return 0;
    if ( ratio >= 1 )
        return 1;
    double t = t_for_x(ratio, bezier_, sample_cache_);
    return bezier_.solve_component(t, 1);
}

double model::KeyframeTransition::bezier_parameter(double ratio) const
{
    if ( ratio <= 0 || hold_ )
        return 0;
    if ( ratio >= 1 )
        return 1;
    return t_for_x(ratio, bezier_, sample_cache_);
}

model::KeyframeTransition::KeyframeTransition(const QPointF& before_handle, const QPointF& after_handle, bool hold)
    : bezier_({0, 0}, before_handle, after_handle, {1,1}),
    hold_(hold)
{}

std::pair<model::KeyframeTransition, model::KeyframeTransition> model::KeyframeTransition::split(double x) const
{
    if ( hold_ )
        return { {{0, 0}, {1, 1}, true}, {{0, 0}, {1, 1}, true} };

    qreal t = t_for_x(x, bezier_, sample_cache_);
    qreal y = bezier_.solve_component(t, 1);
    math::bezier::BezierSegment left, right;
    std::tie(left, right) = bezier_.split(t);

    qreal left_factor_x = 1 / x;
    qreal left_factor_y = 1 / y;
    qreal right_factor_x = 1 / (1-x);
    qreal right_factor_y = 1 / (1-y);
    return {
        {
            {left[1].x() * left_factor_x, left[1].y() * left_factor_y},
            {left[2].x() * left_factor_x, left[2].y() * left_factor_y}
        },
        {
            {(right[1].x() - x) * right_factor_x, (right[1].y() - y) * right_factor_y},
            {(right[2].x() - x) * right_factor_x, (right[2].y() - y) * right_factor_y}
        }
    };
}
