#include <vector>
#include <string>
#include <set>
#include "Block.h"
#include "OperationType.h"

class RankOram {
public:
    RankOram();
    array<int8_t, Block::DATA_SIZE> Access(Operation op, const int address, array<int8_t, Block::DATA_SIZE>& newdata);
    
    // evaluate
    int GetMaxLevel();
    int GetBandwidth();
    void ClearBandwidth();
    void PrintOram();

private:
    int max_level;
    int count;
    int number;
    long bandwidth;
    std::vector<std::vector<Block>> server;
    std::vector<int> is_full;
    std::vector<std::vector<int>> client;
    std::vector<std::vector<int>> sentinel;
    std::vector<int> dummy_counter;

    void Rebuild(const int rebuild_level, const int delete_flag);
    void Evict(int level, std::vector<Block>& eviction, const int delete_flag, std::set<int>& delete_Blocks); 
    // tools
    int Msb(int x);
    void MergeClient(const int rebuild_level, std::vector<int>& merge, const int delete_flag, std::set<int>& delete_Blocks);
    void CreateSentinel(const int level, const std::vector<int>& merge, std::vector<int>& new_sentinel);
    int Index(int rank, int level);
    int Contains(const int x, const int level);
    // a simple permutation: rotate first bit to the last
    int Permutate(int level, int input);
    int InvPermutate(int level, int input);
    int Rank(const int x, const int level);
    void Shuffle(int level, std::vector<Block>& Blocks);
    // @return (level, rank)
    std::pair<int, int> Position(const int x);
};