#include "comp_graph.hpp"

#include <unordered_set>
#include <unordered_map>
#include <deque>

#include "model/document.hpp"
#include "model/assets/assets.hpp"
#include "model/shapes/precomp_layer.hpp"

void model::CompGraph::add_composition(model::Composition* comp)
{
    std::vector<model::PreCompLayer*>& comp_layers = layers[comp];
    std::deque<model::DocumentNode*> nodes(comp->docnode_children().begin(), comp->docnode_children().end());

    while ( !nodes.empty() )
    {
        auto front = nodes.front();
        nodes.pop_front();
        if ( auto layer = front->cast<model::PreCompLayer>() )
            comp_layers.push_back(layer);
        else
            nodes.insert(nodes.end(), front->docnode_children().begin(), front->docnode_children().end());
    }
}

void model::CompGraph::remove_composition(model::Composition* comp)
{
    layers.erase(comp);
}

bool model::CompGraph::is_ancestor_of(model::Composition* ancestor, model::Composition* descendant) const
{
    std::unordered_set<model::Composition*> checked;
    std::unordered_set<model::Composition*> not_checked;
    not_checked.insert(ancestor);

    while ( !not_checked.empty() )
    {
        std::unordered_set<model::Composition*> next;

        for ( model::Composition* comp : not_checked )
        {
            if ( comp == descendant )
                return true;

            auto it = layers.find(comp);
            if ( it == layers.end() )
                continue;

            for ( auto layer : layers.at(comp) )
            {
                auto laycomp = layer->composition.get();
                if ( laycomp && !checked.count(laycomp) )
                    next.insert(laycomp);
            }

            checked.insert(comp);
        }

        not_checked = std::move(next);
    }

    return false;
}

std::vector<model::Composition *> model::CompGraph::children(model::Composition* comp) const
{
    std::unordered_set<model::Composition*> vals;
    for ( auto layer : layers.at(comp) )
    {
        if ( auto laycomp = layer->composition.get() )
            vals.insert(laycomp);
    }

    return std::vector<model::Composition *>(vals.begin(), vals.end());
}

static bool recursive_is_ancestor_of(
    model::Composition* ancestor,
    model::Composition* descendant,
    std::unordered_map<model::Composition*, bool>& cache,
    const std::unordered_map<model::Composition*, std::vector<model::PreCompLayer*>>& layers
)
{
    if ( ancestor == descendant )
        return cache[ancestor] = true;

    auto it = cache.find(ancestor);
    if ( it != cache.end() )
        return it->second;

    int is_ancestor = 0;

    for ( auto layer : layers.at(ancestor) )
    {
        if ( auto laycomp = layer->composition.get() )
            is_ancestor += recursive_is_ancestor_of(laycomp, descendant, cache, layers);
    }

    return cache[ancestor] = is_ancestor;
}

std::vector<model::Composition *> model::CompGraph::possible_descendants(model::Composition* ancestor, model::Document* document) const
{
    std::unordered_map<model::Composition*, bool> cache;
    std::vector<model::Composition*> valid;

    for ( const auto& precomp : document->assets()->precompositions->values )
    {
        if ( !recursive_is_ancestor_of(precomp.get(), ancestor, cache, layers) )
            valid.push_back(precomp.get());
    }

    return valid;
}

void model::CompGraph::add_connection(model::Composition* comp, model::PreCompLayer* layer)
{
    auto it = layers.find(comp);
    if ( it != layers.end() )
        it->second.push_back(layer);
}

void model::CompGraph::remove_connection(model::Composition* comp, model::PreCompLayer* layer)
{
    auto it_map = layers.find(comp);
    if ( it_map != layers.end() )
    {
        auto it_v = std::find(it_map->second.begin(), it_map->second.end(), layer);
        if ( it_v != it_map->second.end() )
        {
            if ( it_v != it_map->second.end() - 1 )
                std::swap(*it_v, it_map->second.back());
            it_map->second.pop_back();
        }
    }
}

