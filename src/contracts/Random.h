using namespace QPI;

struct RANDOM2
{
};

struct RANDOM : public ContractBase
{
public:
    struct GetRandom_input
    {
    };
    struct GetRandom_output
    {
        sint64 millisecond;
        sint64 second;
        sint64 minute;
    };

private:
    sint64 millisecond;
    sint64 second;
    sint64 minute;

    PUBLIC_FUNCTION(GetRandom)
    {
        output.millisecond = qpi.now().millisecond;
        output.second = qpi.now().second;
        output.minute = qpi.now().minute;
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(GetRandom, 1);
    }

    INITIALIZE()
    {
    }
};
