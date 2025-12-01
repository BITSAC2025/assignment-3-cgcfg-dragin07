/**
 * ICFG.cpp
 * @author kisslune 
 */

#include "CFGA.h"

using namespace SVF;
using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    auto moduleNameVec =
            OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                     "[options] <input-bitcode...>");

    LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVFIRBuilder builder;
    auto pag = builder.build();
    auto icfg = pag->getICFG();

    CFGAnalysis analyzer = CFGAnalysis(icfg);

    // TODO: complete the following method: 'CFGAnalysis::analyze'
    analyzer.analyze(icfg);

    analyzer.dumpPaths();
    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}


void CFGAnalysis::analyze(SVF::ICFG *icfg)
{
    for (auto src : sources)
        for (auto snk : sinks)
        {
            while (!callStack.empty()) callStack.pop();

            std::vector<unsigned> path;
            std::set<std::pair<unsigned, std::vector<unsigned>>> onPath;

            auto stackToVector = [&](const std::stack<unsigned>& st) -> std::vector<unsigned> {
                std::vector<unsigned> seq;
                std::stack<unsigned> tmp = st; 
                while (!tmp.empty()) { seq.push_back(tmp.top()); tmp.pop(); }
                std::reverse(seq.begin(), seq.end()); 
                return seq;
            };

            function<void(SVF::ICFGNode*)> dfs = [&](SVF::ICFGNode* node) {
                if (!node) return;

                unsigned nodeId = node->getId();
                auto stateSig = std::make_pair(nodeId, stackToVector(callStack));
                
                // Check if this state is currently on the path (cycle detection)
                if (onPath.find(stateSig) != onPath.end())
                    return;
                
                onPath.insert(stateSig);
                path.push_back(nodeId);

                if (nodeId == snk && callStack.empty())
                {
                    recordPath(path);
                }
                else
                {
                    for (auto edge : node->getOutEdges())
                    {
                        if (auto callEdge = dyn_cast<CallCFGEdge>(edge))
                        {
                            auto dst = callEdge->getDstNode();
                            auto callSite = callEdge->getCallSite();
                            if (callSite) callStack.push(callSite->getId());
                            dfs(dst);
                            if (callSite && !callStack.empty()) callStack.pop();
                        }
                        else if (auto retEdge = dyn_cast<RetCFGEdge>(edge))
                        {
                            auto dst = retEdge->getDstNode();
                            auto callSite = retEdge->getCallSite();
                            if (!callStack.empty() && callSite && callStack.top() == callSite->getId())
                            {
                                callStack.pop();
                                dfs(dst);
                                callStack.push(callSite->getId());
                            }
                        }
                        else
                        {
                            dfs(edge->getDstNode());
                        }
                    }
                }

                // Clean up when backtracking - THIS IS THE KEY FIX
                path.pop_back();
                onPath.erase(stateSig);
            };

            dfs(icfg->getICFGNode(src));
        }
}
