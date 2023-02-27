#pragma once

#include <vector>
#include <cassert>
#include <array>

#include "Vec2.hpp"
#include "Timer.hpp"
#include "ValueGrid.hpp"

#include "EntityManager.hpp"

class World
{
    // world properties
    double m_width = 1920; // width  of the world 
    double m_height = 1080; // height of the world 

    EntityManager   m_entitiyManager;
    std::vector<ValueGrid>         m_grids;

public:

    World(double width, double height)
        : m_width(width)
        , m_height(height)
    {
        
    }

    ~World()
    {
        std::cerr << "World destroyed." << std::endl;
    }

    void update()
    {
        m_entitiyManager.update();
    }

    Entity addEntity(const std::string & tag)
    {
        return m_entitiyManager.addEntity(tag);
    }

    void addGrid(const ValueGrid & grid)
    {
        m_grids.push_back(grid);
    }

    std::vector<Entity> & getEntities()
    {
        return m_entitiyManager.getEntities();
    }

    std::vector<Entity> & getEntities(const std::string & tag)
    {
        return m_entitiyManager.getEntities(tag);
    }
    
    ValueGrid & getGrid(size_t index)
    {
        if (m_grids[index].width() == 0) { throw "Empty grid"; }
        return m_grids[index];
    }

    int getNumberOfGrids()
    {
        return m_grids.size();
    }

    double width() const
    {
        return m_width;
    }

    double height() const
    {
        return m_height;
    }
};