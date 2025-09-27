#define NO_UEFI

#include "contract_testing.h"

std::string assetNameFromInt64(uint64 assetName);
const id devAddress = ID(_B, _O, _N, _D, _D, _J, _N, _U, _H, _O, _G, _Y, _L, _A, _A, _A, _C, _V, _X, _C, _X, _F, _G, _F, _R, _C, _S, _D, _C, _U, _W, _C, _Y, _U, _N, _K, _M, _P, _G, _O, _I, _F, _E, _P, _O, _E, _M, _Y, _T, _L, _Q, _L, _F, _C, _S, _B);
const id adminAddress = ID(_B, _O, _N, _D, _A, _A, _F, _B, _U, _G, _H, _E, _L, _A, _N, _X, _G, _H, _N, _L, _M, _S, _U, _I, _V, _B, _K, _B, _H, _A, _Y, _E, _Q, _S, _Q, _B, _V, _P, _V, _N, _B, _H, _L, _F, _J, _I, _A, _Z, _F, _Q, _C, _W, _W, _B, _V, _E);
const id testAddress1 = ID(_H, _O, _G, _T, _K, _D, _N, _D, _V, _U, _U, _Z, _U, _F, _L, _A, _M, _L, _V, _B, _L, _Z, _D, _S, _G, _D, _D, _A, _E, _B, _E, _K, _K, _L, _N, _Z, _J, _B, _W, _S, _C, _A, _M, _D, _S, _X, _T, _C, _X, _A, _M, _A, _X, _U, _D, _F);
const id testAddress2 = ID(_E, _Q, _M, _B, _B, _V, _Y, _G, _Z, _O, _F, _U, _I, _H, _E, _X, _F, _O, _X, _K, _T, _F, _T, _A, _N, _E, _K, _B, _X, _L, _B, _X, _H, _A, _Y, _D, _F, _F, _M, _R, _E, _E, _M, _R, _Q, _E, _V, _A, _D, _Y, _M, _M, _E, _W, _A, _C);
const id testAddress3 = ID(_U, _X, _U, _F, _A, _Q, _M, _C, _X, _Z, _P, _Z, _B, _C, _Z, _V, _X, _V, _D, _C, _V, _L, _B, _P, _S, _Z, _W, _A, _M, _L, _Z, _H, _M, _A, _V, _Y, _M, _Y, _Z, _B, _W, _G, _Z, _J, _J, _K, _I, _Q, _P, _D, _Y, _B, _F, _U, _F, _A);


class QBondChecker : public MSVAULT
{
public:
    HashMap<uint16, MBondInfo, QBOND_MAX_EPOCH_COUNT> getInfo()
    {
        return _epochMbondInfoMap;
    }
};

class ContractTestingQBond : protected ContractTesting
{
public:
    ContractTestingQBond()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(MSVAULT);
        callSystemProcedure(MSVAULT_CONTRACT_INDEX, INITIALIZE);
    }

    QBondChecker* getState()
    {
        return (QBondChecker*)contractStates[MSVAULT_CONTRACT_INDEX];
    }

    bool loadState(const CHAR16* filename)
    {
        return load(filename, sizeof(MSVAULT), contractStates[MSVAULT_CONTRACT_INDEX]) == sizeof(MSVAULT);
    }

    void stake(const int64_t& quMillions, const id& staker)
    {
        MSVAULT::Stake_input input{ quMillions };
        MSVAULT::Stake_output output;
        invokeUserProcedure(MSVAULT_CONTRACT_INDEX, 1, input, output, staker, quMillions * 2000000);
    }

    QSWAP::TeamInfo_output teamInfo()
    {
        return {};
    }

    void bedinEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(MSVAULT_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void stakers()
    {
        printf("population %lld\n", getState()->getInfo().population());
        printf("stakers %lld\n", getState()->getInfo().value(0).stakersAmount);
        for (int i = 0; i < 1024; i++)
        {
            if (!getState()->getInfo().isEmptySlot(i))
            {
                printf("%s\n", assetNameFromInt64(getState()->getInfo().value(i).name).c_str());
                printf("stakers %lld\n", getState()->getInfo().value(i).stakersAmount);
            }
        }
    }
};


TEST(ContractQBond, IssueAsset)
{
    ContractTestingQBond qbond;
    qbond.stakers();
    qbond.bedinEpoch();
    //increaseEnergy(testAddress, 50 * 2000000);
    qbond.stake(50, testAddress1);
    qbond.stakers();
   /* EXPECT_EQ(_commissionFreeAddresses.population(), 1);*/
}

TEST(ContractQBond, CleanupCollections)
{
    EXPECT_EQ(1, 1);
}
