#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <algorithm>
#include <float.h>

class ValueGrid
{
    size_t m_width = 0;
    size_t m_height = 0;
    std::vector<double> m_values;

    // The value to return for attempts to access out-of-bounds elements.
    double m_outOfBoundsValue;
    
    inline size_t getIndex(size_t x, size_t y) const
    {
        return y * m_width + x;
    }

public:

    ValueGrid() {}
    ValueGrid(size_t width, size_t height, double value = 0.0, double oobv = 0.0)
        : m_width(width)
        , m_height(height)
        , m_values(width*height, value)
        , m_outOfBoundsValue(oobv)
    { }

    /**
     * Load the grid from the given filename which must represent an image format that SFML supports.
     * The red channel of the image will be used as the source for this grid.
     */
    ValueGrid(const std::string &filename, double oobv = 0.0)
        : m_outOfBoundsValue(oobv)
    {
        sf::Image image;
        //std::cerr << "Loading image: " << filename << std::endl;
        image.loadFromFile(filename);
        m_width = image.getSize().x;
        m_height = image.getSize().y;
    
        for (size_t y = 0; y < m_height; y++)
        {
            for (size_t x = 0; x < m_width; x++)
            {
                m_values.push_back( image.getPixel(x, y).r );
            }
        }

        normalize();
    }

    void saveToFile(const std::string &filename)
    {
        sf::Image image;
        image.create(m_width, m_height);
    
        for (size_t y = 0; y < m_height; y++)
        {
            for (size_t x = 0; x < m_width; x++)
            {
                sf::Uint8 v = (sf::Uint8) (255 * m_values[ getIndex(x, y) ]);
                image.setPixel(x, y, sf::Color(v, v, v));
            }
        }

        image.saveToFile(filename);
    }

    inline double get(size_t x, size_t y) const
    {
        if (x >= m_width || y >= m_height) return m_outOfBoundsValue;
        size_t index = getIndex(x, y);

        if (index >= m_values.size()) return m_outOfBoundsValue;
        return m_values[index];
    }

    inline void set(size_t x, size_t y, double value)
    {
        size_t index = getIndex(x, y);
        if (index < m_values.size())
            m_values[index] = value;
    }

    inline void setAll(double value)
    {
        for (int i = 0; i < m_values.size(); i++)
            m_values[i] = value;
    }

    inline void addContour(double contourValue, ValueGrid &valueGrid, double intensity)
    {
        for (int i = 0; i < m_values.size(); i++)
            if (fabs(valueGrid.m_values[i] - contourValue) < 0.0025)
                m_values[i] = intensity;
    }

    inline void normalize()
    {
        // max the min value a 0
        auto minVal = *std::min_element(std::begin(m_values), std::end(m_values));
        for (auto & val : m_values) { val -= minVal; }
        
        // divide everything by the max value
        auto maxVal = *std::max_element(std::begin(m_values), std::end(m_values));

        if (maxVal == 0) { return; }
        for (auto & val : m_values) {  val /= maxVal;  }
    }

    inline void invert()
    {
        for (auto & val : m_values) { val = 1.0 - val; }
    }

    size_t width() const
    {
        return m_width;
    }

    size_t height() const
    {
        return m_height;
    }

    double getMaximumBelow(double below)
    {
        double max = DBL_MIN;
        for (int i = 0; i < m_values.size(); i++)
            if (m_values[i] < below && m_values[i] > max)
                max = m_values[i];
//std::cerr << "max: " << max << std::endl;
        return max;
    }

    double getMinimumAbove(double above)
    {
        double min = DBL_MAX;
        for (int i = 0; i < m_values.size(); i++)
            if (m_values[i] > above && m_values[i] < min)
                min = m_values[i];
//std::cerr << "min: " << min << std::endl;
        return min;
    }

};