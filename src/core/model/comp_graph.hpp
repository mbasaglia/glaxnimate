#pragma once

#include <vector>
#include <unordered_map>

namespace model {

class Composition;
class PreCompLayer;
class Document;

/**
 * \brief Dependency graph between compositions.
 *
 * The graph is a directed acyclic graph, rooted in the main composition of a document.
 * I might not be connected as you could have comps not used anywhere.
 *
 * This graph is used to avoid cyclical dependencies.
 */
class CompGraph
{
public:
    /**
     * \brief Adds a composition and scans it for existing layers
     */
    void add_composition(model::Composition* comp);

    /**
     * \brief Remove a composition from the graph
     */
    void remove_composition(model::Composition* comp);

    /**
     * \brief Registers \p layer to be a layer in \p comp.
     */
    void add_connection(model::Composition* comp, model::PreCompLayer* layer);

    /**
     * \brief Registers \p layer to no longer be a layer in \p comp.
     */
    void remove_connection(model::Composition* comp, model::PreCompLayer* layer);

    /**
     * \brief Returns a list of composition used by \p comp,
     */
    std::vector<model::Composition*> children(model::Composition* comp) const;

    /**
     * \brief Returns whether starting from \p ancestor you can find a path to \p descendant using precomp layers.
     *
     * A comp is considered being ancestor of itself.
     */
    bool is_ancestor_of(model::Composition* ancestor, model::Composition* descendant) const;

    /**
     * \brief Returns a list of compositions that can be added as a child of \p ancestor.
     *
     * Basically all the precomps in \p document that are not ancestors of \p ancestor.
     */
    std::vector<model::Composition*> possible_descendants(model::Composition* ancestor, model::Document* document) const;

private:
    std::unordered_map<model::Composition*, std::vector<model::PreCompLayer*>> layers;
};

} // namespace model
