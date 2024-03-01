#pragma once

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <variant>

#include "Instr.h"

class Codegen {
public:
  Codegen(llvm::Module& mod, llvm::IRBuilder<>& ir_builder, llvm::Value* memory)
      : module_(mod), ir_builder_(ir_builder), memory_(memory) {}

  void generate(const auto& r) {
    for (const Instr::Instruction& i : r) {
      std::visit([&](const auto& i_){
        emit(i_);
      }, i);
    }
  }

private:
  void emit(const Instr::Add& instr) {
    auto* val = ir_builder_.getInt8(instr.value);
    auto* ptrval = ir_builder_.CreateLoad(ir_builder_.getPtrTy(), memory_);
    auto* cell = ir_builder_.CreateLoad(ir_builder_.getInt8Ty(), ptrval);
    auto* sum = ir_builder_.CreateAdd(cell, val);
    ir_builder_.CreateStore(sum, ptrval);
  }

  void emit(const Instr::Shift& instr) {
    auto* val = ir_builder_.getInt64(instr.offset);
    auto* ptrval = ir_builder_.CreateLoad(ir_builder_.getPtrTy(), memory_);
    auto* sum = ir_builder_.CreateAdd(ptrval, val);
    ir_builder_.CreateStore(sum, memory_);
  }

  void emit(const Instr::Print&) {
    auto* ft = llvm::FunctionType::get(ir_builder_.getInt32Ty(), {ir_builder_.getInt32Ty()}, false);
    llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "putchar", module_);
    auto* fn = module_.getFunction("putchar");
    auto* ptrval = ir_builder_.CreateLoad(ir_builder_.getPtrTy(), memory_);
    auto* cell = ir_builder_.CreateLoad(ir_builder_.getInt8Ty(), ptrval);
    ir_builder_.CreateCall(fn, {cell});
  }

  void emit(const Instr::Read&) {
    auto* ft = llvm::FunctionType::get(ir_builder_.getInt32Ty(), {}, false);
    llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "getchar", module_);
    auto* fn = module_.getFunction("getchar");
    auto* ptrval = ir_builder_.CreateLoad(ir_builder_.getPtrTy(), memory_);
    auto* inp = ir_builder_.CreateCall(fn, {});
    ir_builder_.CreateStore(inp, ptrval);
  }

  void emit(const Instr::Loop& instr) {
    auto& context = module_.getContext();
    auto parent = ir_builder_.GetInsertBlock()->getParent();
    llvm::BasicBlock *loop_head = llvm::BasicBlock::Create(context, "loop_head", parent);
    ir_builder_.CreateBr(loop_head);
    ir_builder_.SetInsertPoint(loop_head);
    auto* Ptr_ = ir_builder_.CreateLoad(ir_builder_.getPtrTy(), memory_);
    auto* Cell = ir_builder_.CreateLoad(ir_builder_.getInt8Ty(), Ptr_);
    llvm::Value *Zero = ir_builder_.getInt8(0);
    auto* Cmp = ir_builder_.CreateICmpEQ(Zero, Cell);
    llvm::BasicBlock* loop_body = llvm::BasicBlock::Create(context, "loop_body", parent);
    llvm::BasicBlock* after_loop = llvm::BasicBlock::Create(context, "after_loop", parent);
    ir_builder_.CreateCondBr(Cmp, after_loop, loop_body);
    ir_builder_.SetInsertPoint(loop_body);
    for (const auto& instr : instr.instrs) {
      std::visit([&](const auto& instr){
        emit(instr);
      }, instr);
    }
    ir_builder_.CreateBr(loop_head);
    ir_builder_.SetInsertPoint(after_loop);
  }

  llvm::Module& module_;
  llvm::IRBuilder<>& ir_builder_;
  llvm::Value* memory_;
};
