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

    std::vector<unsigned> realSources;
    std::vector<unsigned> realSinks;

    auto isCallSiteNode = [](SVF::ICFGNode *node) -> bool {
        if (!node) return false;
        for (auto edge : node->getOutEdges()) {
            if (llvm::isa<SVF::CallCFGEdge>(edge))
                return true;
        }
        return false;
    };

    for (unsigned id : sources) {
        SVF::ICFGNode *n = icfg->getICFGNode(id);
        if (isCallSiteNode(n))
            realSources.push_back(id);
    }
    for (unsigned id : sinks) {
        SVF::ICFGNode *n = icfg->getICFGNode(id);
        if (isCallSiteNode(n))
            realSinks.push_back(id);
    }


    if (realSources.empty() || realSinks.empty())
        return;


    for (auto src : realSources)
        for (auto snk : realSinks)
        {

            while (!callStack.empty()) callStack.pop();

            std::vector<unsigned> path;

            std::set<std::pair<unsigned, std::vector<unsigned>>> onPath;

            auto stackToVector = [&](const std::stack<unsigned> &st) -> std::vector<unsigned> {
                std::vector<unsigned> seq;
                std::stack<unsigned> tmp = st;
                while (!tmp.empty()) {
                    seq.push_back(tmp.top());
                    tmp.pop();
                }
                std::reverse(seq.begin(), seq.end());
                return seq;
            };

            std::function<void(SVF::ICFGNode*)> dfs =
                [&](SVF::ICFGNode *node) {
                    if (!node) return;

                    unsigned nodeId = node->getId();
                    auto stateSig = std::make_pair(nodeId, stackToVector(callStack));


                    if (onPath.find(stateSig) != onPath.end())
                        return;

                    onPath.insert(stateSig);
                    path.push_back(nodeId);


                    if (nodeId == snk && callStack.empty()) {
                        recordPath(path);
                    } else {

                        for (auto edge : node->getOutEdges()) {

                            if (auto callEdge = llvm::dyn_cast<SVF::CallCFGEdge>(edge)) {
                                auto dst      = callEdge->getDstNode();
                                auto callSite = callEdge->getCallSite();
                                bool pushed   = false;

                                if (callSite) {
                                    callStack.push(callSite->getId());
                                    pushed = true;
                                }

                                dfs(dst);

                                if (pushed && !callStack.empty())
                                    callStack.pop();
                            }
                            else if (auto retEdge = llvm::dyn_cast<SVF::RetCFGEdge>(edge)) {
                                auto dst      = retEdge->getDstNode();
                                auto callSite = retEdge->getCallSite();


                                if (!callStack.empty() && callSite &&
                                    callStack.top() == callSite->getId()) {

                                    callStack.pop();      
                                    dfs(dst);
                                    callStack.push(callSite->getId());  
                                }

                            }
                            else {

                                auto dst = edge->getDstNode();
                                dfs(dst);
                            }
                        }
                    }


                    path.pop_back();
                    onPath.erase(stateSig);
                };


            dfs(icfg->getICFGNode(src));
        }
}
