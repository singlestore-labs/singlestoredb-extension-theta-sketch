#pragma once

namespace datasketches {

enum agg_state_type { MUTABLE_SKETCH, IMMUTABLE_SKETCH, UNION, INTERSECTION };

class theta_data 
{
    public:
        theta_data()
            : m_type(MUTABLE_SKETCH)
        {
        }

        theta_data(agg_state_type t)
            : m_type(t)
        {
        }

        virtual ~theta_data() = default;

        void set_type(agg_state_type t)
        {
            m_type = t;
        }

        agg_state_type get_type() const
        {
            return m_type;
        }

    private:
        agg_state_type m_type;
};

}

