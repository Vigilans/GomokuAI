#ifndef ZobristKey_h
#define ZobristKey_h

enum COLOR{Empty,Black,White};
//3rd-dim of ZobristTable 0-Empty,1-Black,2-White
std::mt19937 mt(01234567);

// Generates a Randome number from 0 to 2^64-1
unsigned long long int randomInt()
{
    std::uniform_int_distribution<unsigned long long int>
    dist(0, UINT64_MAX);
    return dist(mt);
}

//init ZobristTable


class board{
public:
    COLOR BoardState[15][15];
    unsigned long long int ZobristTable[15][15][3];
    unsigned long long int HashVal;
    board(){
        //init BoardState
        for(int i=0; i<15;i++){
            for(int j=0;j<15;j++){
                BoardState[i][j] = Empty;
            }
        }
        //init ZobristTable
        for(int i=0; i<15;i++){
            for(int j=0;j<15;j++){
                for(int k=0;k<3;k++){
                    ZobristTable[i][j][k] = randomInt();
                }
            }
        }
        //init HashVal
        HashVal = ZobristTable[0][0][Empty];
        for(int i=0;i<15;i++){
            for(int j=0;j<15;j++){
                if(!(i ==0 & j ==0)){
                    HashVal ^= ZobristTable[i][j][Empty];
                }
            }
        }
    }
    
    void updateHashVal(int x,int y,COLOR type){
        COLOR pre = BoardState[x][y];
        HashVal ^= ZobristTable[x][y][pre];
        HashVal ^= ZobristTable[x][y][type];
    }
};

#endif /* ZobristKey_h */
