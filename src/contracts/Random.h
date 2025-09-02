using namespace QPI;

struct RANDOM2
{
};

struct RANDOM : public ContractBase
{
public:
    
private:
    uint64 _mbond;

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
    }

    INITIALIZE()
    {
        state._mbond = 293371593293ULL;
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }

    struct BEGIN_EPOCH_locals
    {
        sint8 p1;
        sint8 p2;
        sint8 p3;
        uint64 currentName;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        state._mbond = 293371593293ULL;

        locals.p1 = 48 + QPI::mod(QPI::div((uint64)qpi.epoch(), 100ULL), 10ULL);
        locals.p2 = 48 + QPI::mod(QPI::div((uint64)qpi.epoch(), 10ULL), 10ULL);
        locals.p3 = 48 + QPI::mod((uint64)qpi.epoch(), 10ULL);

        locals.currentName |= (uint64)locals.p1 << (5 * 8);
        locals.currentName |= (uint64)locals.p2 << (6 * 8);
        locals.currentName |= (uint64)locals.p3 << (7 * 8);

        qpi.issueAsset(locals.currentName, SELF, 0, 1000000, 0);
    }

    struct END_EPOCH_locals
    {
    };
    
    END_EPOCH_WITH_LOCALS()
    {
    }
};
