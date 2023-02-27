#pragma once

#include "Components.hpp"

#include <iostream>
#include <vector>

//const size_t MaxEntities = 20000;
const size_t MaxEntities = 200000;
//const size_t MaxEntities = 2000000; // Got up to 20 trials with 32 robots
//const size_t MaxEntities = 3000000; // Gets up to 30 trials with 32 robots
const size_t MaxComponents = 32;

typedef std::tuple <
    std::vector<CTransform>,
    std::vector<CCircleBody>,
    std::vector<CCircleShape>,
    std::vector<CLineBody>,
    std::vector<CController>,
    std::vector<CSensorArray>,
    std::vector<CRobotType>,
    std::vector<CSteer>,
    std::vector<CControllerVis>,
    std::vector<CVectorIndicator>,
    std::vector<CPlowBody>,
    std::vector<CTerritory>,
    std::vector<CColor>
> EntityData;

class EntityMemoryPool
{
    EntityData  m_data;
    size_t      m_numEntities = 0;
    size_t      m_previousEntityIndex = 0;

    std::vector<std::string>    m_tags;
    std::vector<bool>           m_active;
    std::vector<std::bitset<MaxComponents>> m_hasComponent;

    EntityMemoryPool()
    {
        getData<CTransform>().resize(MaxEntities);
        getData<CCircleBody>().resize(MaxEntities);
        getData<CCircleShape>().resize(MaxEntities);
        getData<CLineBody>().resize(MaxEntities);
        getData<CSensorArray>().resize(MaxEntities);
        getData<CSteer>().resize(MaxEntities);
        getData<CRobotType>().resize(MaxEntities);
        getData<CController>().resize(MaxEntities);
        getData<CControllerVis>().resize(MaxEntities);
        getData<CVectorIndicator>().resize(MaxEntities);
        getData<CPlowBody>().resize(MaxEntities);
        getData<CTerritory>().resize(MaxEntities);
        getData<CColor>().resize(MaxEntities);
        m_hasComponent.resize(MaxEntities);
        m_tags.resize(MaxEntities);
        m_active.resize(MaxEntities);
    }

    size_t getNextEntityIndex()
    {
        size_t goodIndex = 0;
        bool found = false;
        for (size_t i = 0; i < MaxEntities; i++)
        {
            size_t index = (m_previousEntityIndex + i) % MaxEntities;
            if (!m_active[index])
            {
                goodIndex = index;
                found = true;
                break;
            }
        }
        assert(found);
        if (!found) { std::cerr << "WARNING: No valid entity index found\n"; }
        return goodIndex;
    }

public:

    inline static EntityMemoryPool & Instance()
    {
        static EntityMemoryPool instance;
        return instance;
    }

    inline static void Reset()
    {
        /*
        static EntityMemoryPool instance;
        for (int i=0; i<MaxEntities; i++) {
            instance.m_tags[i] = "";
            instance.m_active[i] = false;
            instance.m_hasComponent[i] = {};

            instance.getData<CTransform>()[i] = {};
            instance.getData<CCircleBody>()[i] = {};
            instance.getData<CCircleShape>()[i] = {};
            instance.getData<CLineBody>()[i] = {};
            instance.getData<CSensorArray>()[i] = {};
            instance.getData<CSteer>()[i] = {};
            instance.getData<CRobotType>()[i] = {};
            instance.getData<CController>()[i] = {};
            instance.getData<CControllerVis>()[i] = {};
            instance.getData<CVectorIndicator>()[i] = {};
            instance.getData<CColor>()[i] = {};

        }
        instance.m_numEntities = 0;
        instance.m_previousEntityIndex = 0;
        //std::cerr << "RESET\n";
        */
    }

    inline size_t addEntity(const std::string & tag)
    {
        // look for the next index that contains an inactive entity
        size_t entityIndex = getNextEntityIndex();
        
        getData<CTransform>()[entityIndex]    = {};
        getData<CCircleBody>()[entityIndex]   = {};
        getData<CCircleShape>()[entityIndex]  = {};
        getData<CLineBody>()[entityIndex]     = {};
        getData<CSensorArray>()[entityIndex]  = {};
        getData<CColor>()[entityIndex]        = {};
        getData<CRobotType>()[entityIndex]    = {};
        getData<CController>()[entityIndex]   = {};
        getData<CControllerVis>()[entityIndex]   = {};
        getData<CVectorIndicator>()[entityIndex]   = {};
        getData<CPlowBody>()[entityIndex]   = {};
        getData<CTerritory>()[entityIndex]   = {};
        getData<CSteer>()[entityIndex]        = {};
        m_hasComponent[entityIndex]           = {};
        m_tags[entityIndex]                   = tag;
        m_active[entityIndex]                 = true;

        // return the pointer to the entity
        m_previousEntityIndex = entityIndex;
        return entityIndex;
    }

    inline decltype(m_active) & getActive()
    {
        return m_active;
    }

    inline const decltype(m_tags) & getTags() const
    {
        return m_tags;
    }

    inline decltype(m_hasComponent) & hasComponent() 
    {
        return m_hasComponent;
    }

    template <typename T>
    inline std::vector<T> & getData()
    {
        return std::get<std::vector<T>>(m_data);
    }
};

