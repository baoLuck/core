using namespace QPI;

constexpr uint64 QLOAN_PLACE_LOAN_REQ_FEE = 100000;

constexpr uint64 QLOAN_ACCEPTANCE_FEE_PERCENT = 15;
constexpr uint64 QLOAN_DISTRIBUTE_PERCENT = 5000; // 50%
constexpr uint64 QLOAN_BURN_PERCENT = 500; // 5%
constexpr uint64 QLOAN_QVAULT_PERCENT = 4500; // 45%

constexpr uint64 QLOAN_MAX_LOAN_PERIOD_IN_EPOCHS = 52;
constexpr uint64 QLOAN_MAX_INTEREST_RATE = 100;
constexpr uint64 QLOAN_MAX_ASSETS_NUM = 2;
constexpr uint64 QLOAN_MAX_OUTPUT_NUM = 128;

constexpr uint64 QLOAN_MAX_LOAN_REQS_NUM = 1024;

struct QLOAN2
{
};

struct QLOAN : public ContractBase
{
    enum class LoanReqState : uint8
    {
        IDLE = 1,
        ACTIVE,
        PAYED,
        EXPIRED,
    };

    struct LoanOutputInfo
    {
        id borrower;
        id creditor;
        id acceptedBy;
        id privateId;

        uint64 reqId;

        Array<Asset, QLOAN_MAX_ASSETS_NUM> assets;
        Array<sint64, QLOAN_MAX_ASSETS_NUM> assetAmount;
        uint8 assetsNum;

        uint64 priceAmount;
        uint64 interestRate;
        uint64 debtAmount;

        uint64 returnPeriodInEpochs;
        uint64 epochsLeft;

        enum LoanReqState state;

        bool assetsToCreditor;
    };

    struct LoanReqInfo
    {
        id borrower;
        id creditor;
        // Using these field to know which request user is accepted and which is created by him
        id acceptedBy;
        id privateId;

        Array<Asset, QLOAN_MAX_ASSETS_NUM> assets;
        Array<sint64, QLOAN_MAX_ASSETS_NUM> assetAmount;
        uint8 assetsNum;

        uint64 priceAmount;
        uint64 interestRate;
        uint64 returnPeriodInEpochs;

        uint64 debtAmount;
        uint64 epochsLeft;

        bool assetsToCreditor;

        enum LoanReqState state;
    };

    struct StateData
    {
        HashMap<uint64, struct LoanReqInfo, QLOAN_MAX_LOAN_REQS_NUM> _loanReqs;
        uint64 _totalReqs;
        uint64 _currentLoanIndex;

        uint64 _earnedAmount;
        uint64 _distributedAmount;

        // These two mostly for debug purposes
        uint64 _burnedAmount;
        uint64 _toQvaultAmount;
    };

    struct _TransferAssetsFromTo_input
    {
        LoanReqInfo loanReq;
        id from;
        id to;
    };

    struct _TransferAssetsFromTo_output
    {
    };

    struct _TransferAssetsFromTo_locals
    {
        Asset userReqAsset;
        uint8 userReqAssetIdx;
    };

    PRIVATE_PROCEDURE_WITH_LOCALS(_TransferAssetsFromTo)
    {
        locals.userReqAssetIdx = 0;
        
        while (locals.userReqAssetIdx < input.loanReq.assetsNum)
        {
            locals.userReqAsset = input.loanReq.assets.get(locals.userReqAssetIdx);
            qpi.transferShareOwnershipAndPossession(locals.userReqAsset.assetName, locals.userReqAsset.issuer,
                input.from, input.from,
                input.loanReq.assetAmount.get(locals.userReqAssetIdx),
                input.to);
            locals.userReqAssetIdx++;
        }
    }

    struct _CheckAssetsPresence_input
    {
        Array<Asset, QLOAN_MAX_ASSETS_NUM> assets;
        Array<sint64, QLOAN_MAX_ASSETS_NUM> assetAmount;
        uint8 assetsNum;
    };

    struct _CheckAssetsPresence_output
    {
        bool allGood;
    };

    struct _CheckAssetsPresence_locals
    {
        uint64 loanReqsIdx;
        LoanReqInfo tmpLoanReq;
        Array<sint64, QLOAN_MAX_ASSETS_NUM> userAssetsAmountInvolved;

        uint64 inputReqAssetIdx;
        uint64 tmpReqAssetIdx;
    };

    PRIVATE_FUNCTION_WITH_LOCALS(_CheckAssetsPresence)
    {
        output.allGood = true;

        // Collect all user's tokens involved in others deals
        locals.loanReqsIdx = state.get()._loanReqs.nextElementIndex(NULL_INDEX);
        while (locals.loanReqsIdx != NULL_INDEX)
        {
            locals.tmpLoanReq = state.get()._loanReqs.value(locals.loanReqsIdx);
            // If user is a borrower - we need to check if assets transfered to the creditor in case of Active request,
            // otherwise request might be in the IDLE state and we still should count these tokens
            // If user is a creditor and assets should be transfered to creditor in active loan request then we should count these tokens too
            if ((locals.tmpLoanReq.borrower == qpi.invocator() && ((locals.tmpLoanReq.state == LoanReqState::ACTIVE && locals.tmpLoanReq.assetsToCreditor == false) || locals.tmpLoanReq.state == LoanReqState::IDLE))
                || (locals.tmpLoanReq.creditor == qpi.invocator() && (locals.tmpLoanReq.state == LoanReqState::ACTIVE && locals.tmpLoanReq.assetsToCreditor == true)))
            {
                // Iterate over assets in the user loan request
                locals.inputReqAssetIdx = 0;
                while (locals.inputReqAssetIdx < input.assetsNum)
                {
                    locals.tmpReqAssetIdx = 0;
                    while (locals.tmpReqAssetIdx < locals.tmpLoanReq.assetsNum)
                    {
                        if (input.assets.get(locals.inputReqAssetIdx).assetName == locals.tmpLoanReq.assets.get(locals.tmpReqAssetIdx).assetName)
                        {
                            locals.userAssetsAmountInvolved.set(locals.inputReqAssetIdx,
                                locals.userAssetsAmountInvolved.get(locals.inputReqAssetIdx) + locals.tmpLoanReq.assetAmount.get(locals.tmpReqAssetIdx));
                        }
                        locals.tmpReqAssetIdx++;
                    }
                    locals.inputReqAssetIdx++;
                }
            }
            locals.loanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.loanReqsIdx);
        }
        // Check that user have enough of tokens considering his tokens from others deals
        locals.inputReqAssetIdx = 0;
        while (locals.inputReqAssetIdx < input.assetsNum)
        {
            if (qpi.numberOfPossessedShares(input.assets.get(locals.inputReqAssetIdx).assetName,
                input.assets.get(locals.inputReqAssetIdx).issuer,
                qpi.invocator(), qpi.invocator(),
                SELF_INDEX, SELF_INDEX) - locals.userAssetsAmountInvolved.get(locals.inputReqAssetIdx) < input.assetAmount.get(locals.inputReqAssetIdx))
            {
                output.allGood = false;
                return;
            }
            locals.inputReqAssetIdx++;
        }
    }

    struct _FillLoanReqForOutput_input
    {
        uint64 loanReqId;
        struct LoanReqInfo loanReqInfo;
    };

    struct _FillLoanReqForOutput_output
    {
        LoanOutputInfo loanOutputInfo;
    };

    PRIVATE_FUNCTION(_FillLoanReqForOutput)
    {
        output.loanOutputInfo.borrower = input.loanReqInfo.borrower;
        output.loanOutputInfo.creditor = input.loanReqInfo.creditor;
        output.loanOutputInfo.acceptedBy = input.loanReqInfo.acceptedBy;
        output.loanOutputInfo.privateId = input.loanReqInfo.privateId;
        output.loanOutputInfo.reqId = input.loanReqId;
        output.loanOutputInfo.assets = input.loanReqInfo.assets;
        output.loanOutputInfo.assetAmount = input.loanReqInfo.assetAmount;
        output.loanOutputInfo.assetsNum = input.loanReqInfo.assetsNum;
        output.loanOutputInfo.priceAmount = input.loanReqInfo.priceAmount;
        output.loanOutputInfo.interestRate = input.loanReqInfo.interestRate;
        output.loanOutputInfo.debtAmount = input.loanReqInfo.debtAmount;
        output.loanOutputInfo.returnPeriodInEpochs = input.loanReqInfo.returnPeriodInEpochs;
        output.loanOutputInfo.epochsLeft = input.loanReqInfo.epochsLeft;
        output.loanOutputInfo.assetsToCreditor = input.loanReqInfo.assetsToCreditor;
        output.loanOutputInfo.state = input.loanReqInfo.state;
    }

public:
    struct PlaceLoanReq_input
    {
        // It's for whom these deal
        id privateId;

        Array<Asset, QLOAN_MAX_ASSETS_NUM> assets;
        Array<sint64, QLOAN_MAX_ASSETS_NUM> assetAmount;
        uint8 assetsNum;

        uint64 price;
        uint64 interestRate;
        uint64 returnPeriodInEpochs;

        bool isLoanReq;
        bool assetsToCreditor;
    };

    struct PlaceLoanReq_output
    {
    };

    struct PlaceLoanReq_locals
    {
        struct LoanReqInfo loanReqInfo;
        uint64 loanReqIdIdx;
        Asset userReqAsset;
        uint8 userReqAssetIdx;

        _CheckAssetsPresence_input checkAssetsPresenceInput;
        _CheckAssetsPresence_output checkAssetsPresenceOutput;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(PlaceLoanReq)
    {
        // Check if we have space for new request
        if (state.get()._totalReqs >= QLOAN_MAX_LOAN_REQS_NUM)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Check all inputs are valid
        if (input.returnPeriodInEpochs > QLOAN_MAX_LOAN_PERIOD_IN_EPOCHS
            || input.price == 0
            || input.interestRate == 0
            || input.interestRate > QLOAN_MAX_INTEREST_RATE
            || input.assetsNum == 0
            || input.assetsNum > QLOAN_MAX_ASSETS_NUM)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (input.isLoanReq)
        {
            // Check he have enough qus for FEE
            if (qpi.invocationReward() < QLOAN_PLACE_LOAN_REQ_FEE)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }
            // We still want to get the FEE from the user in both cases(if he have enough assests and NOT)
            else if (qpi.invocationReward() > QLOAN_PLACE_LOAN_REQ_FEE)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward() - QLOAN_PLACE_LOAN_REQ_FEE);
            }
            state.mut()._earnedAmount += QLOAN_PLACE_LOAN_REQ_FEE;

            locals.checkAssetsPresenceInput.assets = input.assets;
            locals.checkAssetsPresenceInput.assetAmount = input.assetAmount;
            locals.checkAssetsPresenceInput.assetsNum = input.assetsNum;
            CALL(_CheckAssetsPresence, locals.checkAssetsPresenceInput, locals.checkAssetsPresenceOutput);

            if (locals.checkAssetsPresenceOutput.allGood == false)
            {
                return;
            }
        }
        // User want to create creditor request, he wants assets, we need to check he have enough QUs
        else
        {
            if (qpi.invocationReward() < sint64(input.price + QLOAN_PLACE_LOAN_REQ_FEE))
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }
            else if (qpi.invocationReward() > sint64(input.price + QLOAN_PLACE_LOAN_REQ_FEE))
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward() - input.price - QLOAN_PLACE_LOAN_REQ_FEE);
            }
            state.mut()._earnedAmount += QLOAN_PLACE_LOAN_REQ_FEE;
        }

        locals.loanReqInfo.borrower = input.isLoanReq ? qpi.invocator() : NULL_ID;
        locals.loanReqInfo.creditor = input.isLoanReq ? NULL_ID : qpi.invocator();
        locals.loanReqInfo.acceptedBy = NULL_ID;
        locals.loanReqInfo.assets = input.assets;
        locals.loanReqInfo.assetAmount = input.assetAmount;
        locals.loanReqInfo.assetsNum = input.assetsNum;
        locals.loanReqInfo.priceAmount = input.price;
        locals.loanReqInfo.interestRate = input.interestRate;
        // until 1/3 of the loan period has passed, the debt will be input.price + 1/3 of the total interest
        locals.loanReqInfo.debtAmount = input.price + div(input.price * input.interestRate, 300ULL);
        locals.loanReqInfo.returnPeriodInEpochs = input.returnPeriodInEpochs;
        locals.loanReqInfo.epochsLeft = input.returnPeriodInEpochs;
        locals.loanReqInfo.privateId = input.privateId;
        locals.loanReqInfo.assetsToCreditor = input.assetsToCreditor;
        locals.loanReqInfo.state = LoanReqState::IDLE;

        state.mut()._loanReqs.set(state.get()._currentLoanIndex, locals.loanReqInfo);
        state.mut()._totalReqs++;
        state.mut()._currentLoanIndex++;
    }

    struct AcceptLoanReq_input
    {
        uint64 reqId;
    };

    struct AcceptLoanReq_output
    {
    };

    struct AcceptLoanReq_locals
    {
        LoanReqInfo tmpLoanReq;

        // To be able to iterate over user's assets
        Asset userReqAsset;
        uint8 userReqAssetIdx;

        _CheckAssetsPresence_input checkAssetsPresenceInput;
        _CheckAssetsPresence_output checkAssetsPresenceOutput;

        _TransferAssetsFromTo_input transferAssetsFromToInput;
        _TransferAssetsFromTo_output transferAssetsFromToOutput;

        sint64 requestedFee;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(AcceptLoanReq)
    {
        if (state.get()._loanReqs.get(input.reqId, locals.tmpLoanReq) == false)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // We need to figure out which request we want to accept(loan or credit)
        // We could do that by checking ID's of the borrower and creditor
        // If borrower is not empty -> loan(need to check that acceptor have enought QU's money)
        // Else creditor is not empty -> credit(need to check that acceptor have enough assets listed in request)

        // Check that request was not created by the qpi.invocator()
        if (locals.tmpLoanReq.borrower == qpi.invocator() || locals.tmpLoanReq.creditor == qpi.invocator())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        // Check that request in IDLE state
        if (locals.tmpLoanReq.state != LoanReqState::IDLE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Check that request is not private or IF private then check that user can accept it
        if (locals.tmpLoanReq.privateId != NULL_ID && locals.tmpLoanReq.privateId != qpi.invocator())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.requestedFee = div(smul(div(smul(locals.tmpLoanReq.priceAmount, locals.tmpLoanReq.interestRate), 100ULL), QLOAN_ACCEPTANCE_FEE_PERCENT), 100ULL);

        // If request was created by the borrower(he wants money for the assets)
        if (locals.tmpLoanReq.borrower != NULL_ID)
        {
            // Check that creditor send us right amount of money for contract
            if (qpi.invocationReward() < sint64(locals.tmpLoanReq.priceAmount))
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }
            locals.tmpLoanReq.creditor = qpi.invocator();
            locals.tmpLoanReq.acceptedBy = qpi.invocator();
            locals.tmpLoanReq.state = LoanReqState::ACTIVE;

            state.mut()._loanReqs.replace(input.reqId, locals.tmpLoanReq);

            state.mut()._earnedAmount += locals.requestedFee;
            // Send coins to the borrower
            qpi.transfer(locals.tmpLoanReq.borrower,
                locals.tmpLoanReq.priceAmount - locals.requestedFee);
            // Send left coins to the creditor
            if (qpi.invocationReward() > sint64(locals.tmpLoanReq.priceAmount))
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.tmpLoanReq.priceAmount);
            }

            // Transfer all assets from request to creditor if request was created like that
            if (locals.tmpLoanReq.assetsToCreditor)
            {
                locals.transferAssetsFromToInput.loanReq = locals.tmpLoanReq;
                locals.transferAssetsFromToInput.from = locals.tmpLoanReq.borrower;
                locals.transferAssetsFromToInput.to = qpi.invocator();
                CALL(_TransferAssetsFromTo, locals.transferAssetsFromToInput, locals.transferAssetsFromToOutput);
            }
        }
        // If request was created by the creditor(he wants asssets for the money)
        else
        {
            locals.checkAssetsPresenceInput.assets = locals.tmpLoanReq.assets;
            locals.checkAssetsPresenceInput.assetAmount = locals.tmpLoanReq.assetAmount;
            locals.checkAssetsPresenceInput.assetsNum = locals.tmpLoanReq.assetsNum;
            CALL(_CheckAssetsPresence, locals.checkAssetsPresenceInput, locals.checkAssetsPresenceOutput);
            if (locals.checkAssetsPresenceOutput.allGood == false)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }

            // Transfer all assets from request to creditor if request was created like that
            if (locals.tmpLoanReq.assetsToCreditor)
            {
                locals.transferAssetsFromToInput.loanReq = locals.tmpLoanReq;
                locals.transferAssetsFromToInput.from = qpi.invocator();
                locals.transferAssetsFromToInput.to = locals.tmpLoanReq.creditor;
                CALL(_TransferAssetsFromTo, locals.transferAssetsFromToInput, locals.transferAssetsFromToOutput);
            }

            locals.tmpLoanReq.borrower = qpi.invocator();
            locals.tmpLoanReq.acceptedBy = qpi.invocator();
            locals.tmpLoanReq.state = LoanReqState::ACTIVE;
            state.mut()._loanReqs.replace(input.reqId, locals.tmpLoanReq);

            state.mut()._earnedAmount += locals.requestedFee;
            qpi.transfer(qpi.invocator(), qpi.invocationReward() + locals.tmpLoanReq.priceAmount - locals.requestedFee);
        }
    }

    struct RemoveLoanReq_input
    {
        uint64 reqId;
    };

    struct RemoveLoanReq_output
    {
    };

    struct RemoveLoanReq_locals
    {
        sint64 activeLoanReqIdx;
        LoanReqInfo tmpLoanReq;
        Asset userReqAsset;
        uint8 userReqAssetIdx;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(RemoveLoanReq)
    {
        if (state.get()._loanReqs.get(input.reqId, locals.tmpLoanReq) == false)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if ((locals.tmpLoanReq.borrower != qpi.invocator() && locals.tmpLoanReq.creditor != qpi.invocator())
            || locals.tmpLoanReq.state != LoanReqState::IDLE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Need to figure out who is user in this request.
        // He might be a borrower and creditor. If he is a borrower, we need to send all his tokens back.
        // If he is a creditor, we need to send we need to send him back his QUs.
        if (locals.tmpLoanReq.creditor == qpi.invocator())
        {
            // Send him back all his QUs
            qpi.transfer(qpi.invocator(), qpi.invocationReward() + locals.tmpLoanReq.priceAmount);
        }

        // Remove loan req req from the state
        state.mut()._loanReqs.removeByKey(input.reqId);
        state.mut()._totalReqs--;
    }

    struct TransferShareManagementRights_input
    {
        Asset asset;
        sint64 numberOfShares;
        uint32 newManagingContractIndex;
    };

    struct TransferShareManagementRights_output
    {
    };

    struct TransferShareManagementRights_locals
    {
        sint64 loanReqsIdx;
        LoanReqInfo tmpLoanReq;

        sint64 releaseReturnedFee;
        uint64 userAssetsAmountInHold;

        Asset userReqAsset;
        uint8 userReqAssetIdx;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
    {
        if (qpi.invocationReward() == 0)
        {
            return;
        }

        locals.userAssetsAmountInHold = 0;
        locals.loanReqsIdx = state.get()._loanReqs.nextElementIndex(NULL_INDEX);
        while (locals.loanReqsIdx != NULL_INDEX)
        {
            locals.tmpLoanReq = state.get()._loanReqs.value(locals.loanReqsIdx);
            if (locals.tmpLoanReq.borrower == qpi.invocator()
                && ((locals.tmpLoanReq.state == LoanReqState::ACTIVE && locals.tmpLoanReq.assetsToCreditor == false)
                    || locals.tmpLoanReq.state == LoanReqState::IDLE))
            {
                // Need to scan all assets in loan request to make sure it's have an assets user want to release
                locals.userReqAssetIdx = 0;
                locals.userReqAsset = locals.tmpLoanReq.assets.get(locals.userReqAssetIdx);
                while (locals.userReqAssetIdx < locals.tmpLoanReq.assetsNum)
                {
                    if (locals.userReqAsset.assetName == input.asset.assetName)
                    {
                        locals.userAssetsAmountInHold += locals.tmpLoanReq.assetAmount.get(locals.userReqAssetIdx);
                    }
                    locals.userReqAssetIdx++;
                    locals.userReqAsset = locals.tmpLoanReq.assets.get(locals.userReqAssetIdx);
                }
            }
            locals.loanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.loanReqsIdx);
        }

        if (qpi.numberOfPossessedShares(input.asset.assetName,
            input.asset.issuer,
            qpi.invocator(), qpi.invocator(),
            SELF_INDEX, SELF_INDEX) - sint64(locals.userAssetsAmountInHold) >= input.numberOfShares)
        {
            locals.releaseReturnedFee = qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(),
                input.numberOfShares, input.newManagingContractIndex,
                input.newManagingContractIndex, qpi.invocationReward());
            // Check for success
            if (locals.releaseReturnedFee >= 0)
            {
                qpi.transfer(qpi.invocator(), locals.releaseReturnedFee);
            }
            else
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
        }
        else
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
    }

    struct PayLoanDebt_input
    {
        uint64 reqId;
    };

    struct PayLoanDebt_output
    {
    };

    struct PayLoanDebt_locals
    {
        sint64 loanReqsIdx;
        LoanReqInfo tmpLoanReq;
        bool debtPayed;

        uint64 debtAmount;

        _TransferAssetsFromTo_input transferAssetsFromToInput;
        _TransferAssetsFromTo_output transferAssetsFromToOutput;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(PayLoanDebt)
    {
        if (state.get()._loanReqs.get(input.reqId, locals.tmpLoanReq) == false)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Make sure we check all conditions before moving forward
        if (locals.tmpLoanReq.borrower != qpi.invocator()
            || locals.tmpLoanReq.state != LoanReqState::ACTIVE
            || sint64(locals.tmpLoanReq.debtAmount) > qpi.invocationReward()
            || (locals.tmpLoanReq.returnPeriodInEpochs - locals.tmpLoanReq.epochsLeft) < locals.tmpLoanReq.returnPeriodInEpochs / 3)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Send all money with percentage to the creditor from borrower
        qpi.transfer(locals.tmpLoanReq.creditor, locals.tmpLoanReq.debtAmount);

        if (qpi.invocationReward() > sint64(locals.tmpLoanReq.debtAmount))
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.tmpLoanReq.debtAmount);
        }

        // Transfer all assets back to the borrower if it was transfered to the creditor
        if (locals.tmpLoanReq.assetsToCreditor)
        {
            locals.transferAssetsFromToInput.loanReq = locals.tmpLoanReq;
            locals.transferAssetsFromToInput.from = locals.tmpLoanReq.creditor;
            locals.transferAssetsFromToInput.to = locals.tmpLoanReq.borrower;
            CALL(_TransferAssetsFromTo, locals.transferAssetsFromToInput, locals.transferAssetsFromToOutput);
        }

        state.mut()._loanReqs.removeByKey(input.reqId);
        state.mut()._totalReqs--;
    }

    struct GetAllLoanReqs_input
    {
    };

    struct GetAllLoanReqs_output
    {
        Array<LoanOutputInfo, QLOAN_MAX_OUTPUT_NUM> reqs;
        uint64 reqsAmount;
    };

    struct GetAllLoanReqs_locals
    {
        struct LoanReqInfo tmpLoanReqInfo;

        sint64 outputLoanReqsIdx;
        sint64 activeLoanReqsIdx;

        _FillLoanReqForOutput_input fillLoanReqForOutputInput;
        _FillLoanReqForOutput_output fillLoanReqForOutputOutput;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetAllLoanReqs)
    {
        output.reqsAmount = 0;
        locals.outputLoanReqsIdx = 0;

        locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(NULL_INDEX);

        while (locals.activeLoanReqsIdx != NULL_INDEX && locals.outputLoanReqsIdx < 256)
        {
            locals.tmpLoanReqInfo = state.get()._loanReqs.value(locals.activeLoanReqsIdx);

            locals.fillLoanReqForOutputInput.loanReqId = state.get()._loanReqs.key(locals.activeLoanReqsIdx);
            locals.fillLoanReqForOutputInput.loanReqInfo = locals.tmpLoanReqInfo;
            CALL(_FillLoanReqForOutput, locals.fillLoanReqForOutputInput, locals.fillLoanReqForOutputOutput);

            output.reqs.set(locals.outputLoanReqsIdx, locals.fillLoanReqForOutputOutput.loanOutputInfo);
            locals.outputLoanReqsIdx++;
            output.reqsAmount++;

            locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.activeLoanReqsIdx);
        }
    }

    struct GetUserActiveLoanReqs_input
    {
        id userId;
    };

    struct GetUserActiveLoanReqs_output
    {
        Array<LoanOutputInfo, QLOAN_MAX_OUTPUT_NUM> reqs;
        uint64 reqsAmount;
    };

    struct GetUserActiveLoanReqs_locals
    {
        struct LoanReqInfo tmpLoanReqInfo;

        sint64 outputLoanReqsIdx;
        sint64 activeLoanReqsIdx;

        _FillLoanReqForOutput_input fillLoanReqForOutputInput;
        _FillLoanReqForOutput_output fillLoanReqForOutputOutput;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetUserActiveLoanReqs)
    {
        output.reqsAmount = 0;
        locals.outputLoanReqsIdx = 0;

        locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(NULL_INDEX);

        while (locals.activeLoanReqsIdx != NULL_INDEX && locals.outputLoanReqsIdx < 256)
        {
            locals.tmpLoanReqInfo = state.get()._loanReqs.value(locals.activeLoanReqsIdx);
            if ((locals.tmpLoanReqInfo.borrower == input.userId || locals.tmpLoanReqInfo.creditor == input.userId)
                && locals.tmpLoanReqInfo.state == LoanReqState::ACTIVE)
            {
                locals.fillLoanReqForOutputInput.loanReqId = state.get()._loanReqs.key(locals.activeLoanReqsIdx);
                locals.fillLoanReqForOutputInput.loanReqInfo = locals.tmpLoanReqInfo;
                CALL(_FillLoanReqForOutput, locals.fillLoanReqForOutputInput, locals.fillLoanReqForOutputOutput);

                output.reqs.set(locals.outputLoanReqsIdx, locals.fillLoanReqForOutputOutput.loanOutputInfo);
                locals.outputLoanReqsIdx++;
                output.reqsAmount++;
            }

            locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.activeLoanReqsIdx);
        }
    }

    struct GetFeesInfo_input
    {
    };

    struct GetFeesInfo_output
    {
        uint64 acceptanceFeePercent;
        uint64 distributeFeePercent;
        uint64 burnFeePercent;

        uint64 earnedAmount;
        uint64 distributedAmount;
        uint64 burnedAmount;
        uint64 toQvaultAmount;
    };


    PUBLIC_FUNCTION(GetFeesInfo)
    {
        output.acceptanceFeePercent = QLOAN_ACCEPTANCE_FEE_PERCENT;
        output.distributeFeePercent = QLOAN_DISTRIBUTE_PERCENT;
        output.burnFeePercent = QLOAN_BURN_PERCENT;

        output.earnedAmount = state.get()._earnedAmount;
        output.distributedAmount = state.get()._distributedAmount;
        output.burnedAmount = state.get()._burnedAmount;
        output.toQvaultAmount = state.get()._toQvaultAmount;
    }

    struct GetUserDebt_input
    {
        id userId;
    };

    struct GetUserDebt_output
    {
        uint64 totalUserDebt;
    };

    struct GetUserDebt_locals
    {
        struct LoanReqInfo tmpLoanReqInfo;
        sint64 activeLoanReqsIdx;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetUserDebt)
    {
        locals.activeLoanReqsIdx = 0;
        locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(NULL_INDEX);

        while (locals.activeLoanReqsIdx != NULL_INDEX)
        {
            locals.tmpLoanReqInfo = state.get()._loanReqs.value(locals.activeLoanReqsIdx);
            if (locals.tmpLoanReqInfo.borrower == input.userId && locals.tmpLoanReqInfo.state == LoanReqState::ACTIVE)
            {
                output.totalUserDebt += locals.tmpLoanReqInfo.debtAmount;
            }
            locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.activeLoanReqsIdx);
        }
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(PlaceLoanReq, 1);
        REGISTER_USER_PROCEDURE(RemoveLoanReq, 2);
        REGISTER_USER_PROCEDURE(AcceptLoanReq, 3);
        REGISTER_USER_PROCEDURE(TransferShareManagementRights, 4);
        REGISTER_USER_PROCEDURE(PayLoanDebt, 5);

        REGISTER_USER_FUNCTION(GetAllLoanReqs, 1);
        REGISTER_USER_FUNCTION(GetUserActiveLoanReqs, 2);
        REGISTER_USER_FUNCTION(GetUserDebt, 3);
        REGISTER_USER_FUNCTION(GetFeesInfo, 4);
        
    }

    struct BEGIN_EPOCH_locals
    {
        sint64 activeLoanReqsIdx;
        LoanReqInfo tmpLoanReqInfo;
        Asset userReqAsset;
        uint8 userReqAssetIdx;

        _TransferAssetsFromTo_input transferAssetsFromToInput;
        _TransferAssetsFromTo_output transferAssetsFromToOutput;
    };


    BEGIN_EPOCH_WITH_LOCALS()
    {
        locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(NULL_INDEX);
        while (locals.activeLoanReqsIdx != NULL_INDEX)
        {
            locals.tmpLoanReqInfo = state.get()._loanReqs.value(locals.activeLoanReqsIdx);

            // Check if contract already ends
            if (locals.tmpLoanReqInfo.epochsLeft == 0
                && locals.tmpLoanReqInfo.state == LoanReqState::ACTIVE)
            {
                locals.transferAssetsFromToInput.loanReq = locals.tmpLoanReqInfo;

                // Check if assets were NOT transfered to the creditor, if not - transfer assets from borrower to the creditor
                if (locals.tmpLoanReqInfo.assetsToCreditor == false)
                {
                    locals.transferAssetsFromToInput.from = locals.tmpLoanReqInfo.borrower;
                    locals.transferAssetsFromToInput.to = locals.tmpLoanReqInfo.creditor;
                    CALL(_TransferAssetsFromTo, locals.transferAssetsFromToInput, locals.transferAssetsFromToOutput);
                }

                state.mut()._loanReqs.removeByKey(state.get()._loanReqs.key(locals.activeLoanReqsIdx));
                state.mut()._totalReqs--;
            }
            else if (locals.tmpLoanReqInfo.state == LoanReqState::ACTIVE)
            {
                locals.tmpLoanReqInfo.epochsLeft--;
                
                if (div((locals.tmpLoanReqInfo.returnPeriodInEpochs - locals.tmpLoanReqInfo.epochsLeft) * 100, locals.tmpLoanReqInfo.returnPeriodInEpochs) > 33)
                {
                    locals.tmpLoanReqInfo.debtAmount = div(smul(locals.tmpLoanReqInfo.priceAmount,
                                                                    sadd(1000000ULL,
                                                                        div(smul(10000ULL, smul(locals.tmpLoanReqInfo.interestRate,
                                                                            locals.tmpLoanReqInfo.returnPeriodInEpochs - locals.tmpLoanReqInfo.epochsLeft)),
                                                                            locals.tmpLoanReqInfo.returnPeriodInEpochs))), 1000000ULL);
                }

                state.mut()._loanReqs.replace(state.get()._loanReqs.key(locals.activeLoanReqsIdx), locals.tmpLoanReqInfo);
            }

            locals.activeLoanReqsIdx = state.get()._loanReqs.nextElementIndex(locals.activeLoanReqsIdx);
        }
    }

    struct END_EPOCH_locals
    {
        uint64 amountToDistribute;
        uint64 amountToBurn;
        uint64 amountToQvault;
    };

    END_EPOCH_WITH_LOCALS()
    {
        locals.amountToDistribute = div(smul((state.get()._earnedAmount - state.get()._distributedAmount), QLOAN_DISTRIBUTE_PERCENT), 10000ULL);
        locals.amountToBurn = div(smul((state.get()._earnedAmount - state.get()._distributedAmount), QLOAN_BURN_PERCENT), 10000ULL);
        locals.amountToQvault = div(smul((state.get()._earnedAmount - state.get()._distributedAmount), QLOAN_QVAULT_PERCENT), 10000ULL);

        if ((QPI::div(locals.amountToDistribute, 676ULL) > 0) && (state.get()._earnedAmount > state.get()._distributedAmount))
        {
            if (qpi.distributeDividends(QPI::div(locals.amountToDistribute, 676ULL)))
            {
                qpi.burn(locals.amountToBurn);
                qpi.transfer(id(QVAULT_CONTRACT_INDEX, 0, 0, 0), locals.amountToQvault);
                state.mut()._distributedAmount += QPI::div(locals.amountToDistribute, 676ULL) * NUMBER_OF_COMPUTORS;
                state.mut()._distributedAmount += locals.amountToBurn;
                state.mut()._distributedAmount += locals.amountToQvault;

                state.mut()._burnedAmount += locals.amountToBurn;
                state.mut()._toQvaultAmount += locals.amountToQvault;
            }
        }
    }

    INITIALIZE()
    {
        state.mut()._currentLoanIndex = 1;
        state.mut()._totalReqs = 0;

        state.mut()._earnedAmount = 0;
        state.mut()._distributedAmount = 0;
        state.mut()._burnedAmount = 0;
        state.mut()._toQvaultAmount = 0;
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }

    PRE_RELEASE_SHARES()
    {
        output.allowTransfer = true;
    }
};
