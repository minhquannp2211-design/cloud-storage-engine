#include "NamespaceNode.h"
#include <algorithm>

std::string NamespaceNode::GetFullPath() const
{
    std::string path;
    std::vector<std::string> parts;
    
    // Walk up to root, collect names
    auto node = this;
    while (node != nullptr)
    {
        if (!node->name.empty())
            parts.push_back(node->name);
        node = node->parent.get();
    }
    
    // Reverse to get top-down order
    std::reverse(parts.begin(), parts.end());
    
    // Build path with /
    for (size_t i = 0; i < parts.size(); ++i)
    {
        path += parts[i];
        if (i < parts.size() - 1)
            path += "/";
    }
    
    return path;
}

std::shared_ptr<NamespaceNode> NamespaceNode::FindChild(const std::string& name) const
{
    auto it = children.find(name);
    if (it != children.end())
        return it->second;
    return nullptr;
}

std::shared_ptr<NamespaceNode> NamespaceNode::GetOrCreateChild(const std::string& name, bool isFile)
{
    auto it = children.find(name);
    if (it != children.end())
        return it->second;
    
    auto child = std::make_shared<NamespaceNode>(name, isFile);
    child->parent = std::make_shared<NamespaceNode>(*this);
    children[name] = child;
    return child;
}
