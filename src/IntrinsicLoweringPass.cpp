/*
 * GenMC -- Generic Model Checking.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-3.0.html.
 *
 * Author: Michalis Kokologiannakis <michalis@mpi-sws.org>
 */

#include "config.h"
#include "IntrinsicLoweringPass.hpp"
#include <llvm/ADT/Twine.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

bool IntrinsicLoweringPass::runOnBasicBlock(llvm::BasicBlock &BB, llvm::Module &M)
{
	bool modified = false;
	for (auto it = BB.begin(); it != BB.end();  ) {
		llvm::IntrinsicInst *I = llvm::dyn_cast<llvm::IntrinsicInst>(&*it);
		/* Iterator is incremented in order for it not to be invalidated */
		++it;
		/* If the instruction is not an intrinsic call, skip it */
		if (!I)
			continue;
		switch (I->getIntrinsicID()) {
		case llvm::Intrinsic::vastart:
		case llvm::Intrinsic::vaend:
		case llvm::Intrinsic::vacopy:
			break;
		case llvm::Intrinsic::dbg_value:
		case llvm::Intrinsic::dbg_declare:
			/* Remove useless calls to @llvm.debug.* */
			I->eraseFromParent();
			modified = true;
			break;
		case llvm::Intrinsic::trap: {
			/* Lower calls to @llvm.trap to abort() calls */
			auto FC = M.getOrInsertFunction("abort", llvm::Type::getVoidTy(M.getContext()));
#ifdef LLVM_GETORINSERTFUNCTION_RET_FUNCTION
			if (auto *F = llvm::dyn_cast<llvm::Function>(FC)) {
#else
			if (auto *F = llvm::dyn_cast<llvm::Function>(FC.getCallee())) {
#endif
				F->setDoesNotReturn();
				F->setDoesNotThrow();
				llvm::CallInst::Create(F, llvm::Twine(), I);
			}
			new llvm::UnreachableInst(M.getContext(), I);
			I->eraseFromParent();
			modified = true;
			break;
		}
		default:
            const llvm::Function *Callee = I->getCalledFunction();

                if (Callee->getIntrinsicID() == llvm::Intrinsic::objectsize) {
                    llvm::Value *replacement;
                    llvm::IntegerType *intType = llvm::dyn_cast<llvm::IntegerType>(I->getType());
                    assert(intType && "intrinsic does not have integer return type");

                    llvm::Value *minArg = I->getArgOperand(1);
                    assert(minArg && "Failed to get second argument");
                    llvm::ConstantInt *minArgAsInt = llvm::dyn_cast<llvm::ConstantInt>(minArg);
                    assert(minArgAsInt && "Second arg is not a ConstantInt");
                    assert(minArgAsInt->getBitWidth() == 1 &&
                           "Second argument is not an i1");

                    if (minArgAsInt->isZero()) {
                        // min=false
                        replacement = llvm::ConstantInt::get(intType, -1, /*isSigned=*/true);
                    } else {
                        // min=true
                        replacement = llvm::ConstantInt::get(intType, 0, /*isSigned=*/false);
                    }
                    I->replaceAllUsesWith(replacement);
                    I->eraseFromParent();
                    modified = true;

                    break;
                } else if (Callee->getIntrinsicID() == llvm::Intrinsic::umul_with_overflow) {

                    using namespace llvm;

                    IRBuilder<> builder(I->getParent(), I->getIterator());

                    Value *op1 = I->getArgOperand(0);
                    Value *op2 = I->getArgOperand(1);

                    Value *result = 0;
                    Value *result_ext = 0;
                    Value *overflow = 0;

                    unsigned int bw = op1->getType()->getPrimitiveSizeInBits();
                    unsigned int bw2 = op1->getType()->getPrimitiveSizeInBits() * 2;

                    if ((I->getIntrinsicID() == Intrinsic::uadd_with_overflow) ||
                        (I->getIntrinsicID() == Intrinsic::usub_with_overflow) ||
                        (I->getIntrinsicID() == Intrinsic::umul_with_overflow)) {

                        Value *op1ext =
                                builder.CreateZExt(op1, IntegerType::get(M.getContext(), bw2));
                        Value *op2ext =
                                builder.CreateZExt(op2, IntegerType::get(M.getContext(), bw2));
                        Value *int_max_s =
                                ConstantInt::get(op1->getType(), APInt::getMaxValue(bw));
                        Value *int_max = builder.CreateZExt(
                                int_max_s, IntegerType::get(M.getContext(), bw2));

                        if (I->getIntrinsicID() == Intrinsic::uadd_with_overflow) {
                            result_ext = builder.CreateAdd(op1ext, op2ext);
                        } else if (I->getIntrinsicID() == Intrinsic::usub_with_overflow) {
                            result_ext = builder.CreateSub(op1ext, op2ext);
                        } else if (I->getIntrinsicID() == Intrinsic::umul_with_overflow) {
                            result_ext = builder.CreateMul(op1ext, op2ext);
                        }
                        overflow = builder.CreateICmpUGT(result_ext, int_max);

                    } else if ((I->getIntrinsicID() == Intrinsic::sadd_with_overflow) ||
                               (I->getIntrinsicID() == Intrinsic::ssub_with_overflow) ||
                               (I->getIntrinsicID() == Intrinsic::smul_with_overflow)) {

                        Value *op1ext =
                                builder.CreateSExt(op1, IntegerType::get(M.getContext(), bw2));
                        Value *op2ext =
                                builder.CreateSExt(op2, IntegerType::get(M.getContext(), bw2));
                        Value *int_max_s =
                                ConstantInt::get(op1->getType(), APInt::getSignedMaxValue(bw));
                        Value *int_min_s =
                                ConstantInt::get(op1->getType(), APInt::getSignedMinValue(bw));
                        Value *int_max = builder.CreateSExt(
                                int_max_s, IntegerType::get(M.getContext(), bw2));
                        Value *int_min = builder.CreateSExt(
                                int_min_s, IntegerType::get(M.getContext(), bw2));

                        if (I->getIntrinsicID() == Intrinsic::sadd_with_overflow) {
                            result_ext = builder.CreateAdd(op1ext, op2ext);
                        } else if (I->getIntrinsicID() == Intrinsic::ssub_with_overflow) {
                            result_ext = builder.CreateSub(op1ext, op2ext);
                        } else if (I->getIntrinsicID() == Intrinsic::smul_with_overflow) {
                            result_ext = builder.CreateMul(op1ext, op2ext);
                        }
                        overflow =
                                builder.CreateOr(builder.CreateICmpSGT(result_ext, int_max),
                                                 builder.CreateICmpSLT(result_ext, int_min));
                    }

                    // This trunc cound be replaced by a more general trunc replacement
                    // that allows to detect also undefined behavior in assignments or
                    // overflow in operation with integers whose dimension is smaller than
                    // int's dimension, e.g.
                    //     uint8_t = uint8_t + uint8_t;
                    // if one desires the wrapping should write
                    //     uint8_t = (uint8_t + uint8_t) & 0xFF;
                    // before this, must check if it has side effects on other operations
                    result = builder.CreateTrunc(result_ext, op1->getType());
                    Value *resultStruct = builder.CreateInsertValue(
                            UndefValue::get(I->getType()), result, 0);
                    resultStruct = builder.CreateInsertValue(resultStruct, overflow, 1);

                    I->replaceAllUsesWith(resultStruct);
                    I->eraseFromParent();
                    modified = true;


                    break;
                }

			IL->LowerIntrinsicCall(I);
			modified = true;
			break;
		}
	}
	return modified;
}

bool IntrinsicLoweringPass::runOnModule(llvm::Module &M)
{
	bool modified = false;
	/* Scan through the instructions and lower intrinsic calls */
	for (auto &F : M)
		for (auto &BB : F)
			modified |= runOnBasicBlock(BB, M);

	/*
	 * Check for calls to @llvm.trap. Such calls may occur if LLVM
	 * detects a NULL pointer dereference in the CFG and simplify
	 * it to a trap call. In order for this to happen, the program
	 * has to be compiled with -O1 or -O2.
	 */
	if (llvm::Function *trapDecl = M.getFunction("llvm.trap")) {
		trapDecl->eraseFromParent();
		modified = true;
	}
	return modified;
}

char IntrinsicLoweringPass::ID = 42;
