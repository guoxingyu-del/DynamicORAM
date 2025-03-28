#include <set>
#include <vector>
#include <string>

class PerformanceTester {
public:
    const static std::string FIELNAME;
    PerformanceTester();
    void runCorrectnessTest();

private:
    std::vector<std::vector<std::string>> data;
    std::set<std::pair<std::string, int>> database;

    void PathOramTest();
    void CircuitOramTest();
    void DynamicPathOramTest();
    void DynamicCircuitOramTest();
};