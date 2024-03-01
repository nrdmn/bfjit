#include <fmt/format.h>
#include <fstream>
#include <iterator>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <optional>
#include <span>
#include <vector>

#include "CodeGen.h"
#include "Instr.h"
#include "NoncanonicalMode.h"


using namespace llvm;


namespace {

llvm::ExitOnError ExitOnErr;

std::optional<std::vector<Instr::Instruction>> parse(std::span<const char> sp) {
  // TODO: use deducing this
  auto impl = [](const auto& self, std::span<const char>& sp) -> std::optional<std::vector<Instr::Instruction>> {
    std::vector<Instr::Instruction> result;
    while (!sp.empty()) {
      switch (sp[0]) {
      case '+':
        result.push_back(Instr::Add{1});
        break;
      case '-':
        result.push_back(Instr::Add{255});
        break;
      case '>':
        result.push_back(Instr::Shift{1});
        break;
      case '<':
        result.push_back(Instr::Shift{-1});
        break;
      case '.':
        result.push_back(Instr::Print{});
        break;
      case ',':
        result.push_back(Instr::Read{});
        break;
      case '[':
        sp = sp.subspan(1);
        if (auto l = self(self, sp); l && sp.size() != 0 && sp[0] == ']') {
          result.push_back(Instr::Loop{*l});
        } else {
          return std::nullopt;
        }
        break;
      case ']':
        return result;
      default:
        // skip
      }
      sp = sp.subspan(1);
    }
    return result;
  };
  return impl(impl, sp);
}


void print(const std::vector<Instr::Instruction>& instrs, int indent = 0) {
  for (const auto& instr : instrs) {
    std::visit([&indent]<typename T>(const T& instr){
      if constexpr (std::is_same_v<T, Instr::Add>) {
        fmt::print("{:>{}}Add {}\n", "", indent, instr.value);
      } else if constexpr (std::is_same_v<T, Instr::Shift>) {
        fmt::print("{:>{}}Shift {}\n", "", indent, instr.offset);
      } else if constexpr (std::is_same_v<T, Instr::Print>) {
        fmt::print("{:>{}}Print\n", "", indent);
      } else if constexpr (std::is_same_v<T, Instr::Read>) {
        fmt::print("{:>{}}Read\n", "", indent);
      } else if constexpr (std::is_same_v<T, Instr::Loop>) {
        fmt::print("{:>{}}Loop\n", "", indent);
        print(instr.instrs, indent+4);
      }
    }, instr);
  }
}


orc::ThreadSafeModule createBfModule(const std::vector<Instr::Instruction>& instrs) {
  auto Context = std::make_unique<LLVMContext>();
  auto M = std::make_unique<Module>("bf", *Context);

  IRBuilder<> builder(*Context);

  Function *run_bf =
      Function::Create(FunctionType::get(builder.getVoidTy(),
                                         {builder.getPtrTy()}, false),
                       Function::ExternalLinkage, "bf", M.get());

  BasicBlock *BB = BasicBlock::Create(*Context, "EntryBlock", run_bf);
  builder.SetInsertPoint(BB);

  auto* Ptr = builder.CreateAlloca(builder.getPtrTy());
  Argument *Arg = &*run_bf->arg_begin();
  builder.CreateStore(Arg, Ptr);

  Codegen codegen(*M, builder, Ptr);
  codegen.generate(instrs);

  builder.CreateRetVoid();
  return orc::ThreadSafeModule(std::move(M), std::move(Context));
}

}  // namespace


int main(int argc, char* argv[]) {
  InitLLVM LLVM(argc, argv);
  cl::opt<std::string> filename(cl::Positional, cl::Required, cl::desc("<bf file>"));
  cl::opt<bool> dump_ast("dump-ast", cl::init(false), cl::desc("Dump AST"));

  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();

  cl::ParseCommandLineOptions(argc, argv, "bf");
  ExitOnErr.setBanner(std::string(argv[0]) + ": ");

  NoncanonicalMode ncm;

  std::ifstream file{filename.getValue()};
  auto ast = parse(std::vector<char>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()));
  if (!ast) {
    fmt::println("parse error");
    return EXIT_FAILURE;
  }

  if (dump_ast.getValue()) {
    print(*ast);
    return EXIT_SUCCESS;
  }

  auto JITBuilder = ExitOnErr(orc::LLJITBuilder().create());
  auto Module = createBfModule(*ast);

  ExitOnErr(JITBuilder->addIRModule(std::move(Module)));

  auto BfAddr = ExitOnErr(JITBuilder->lookup("bf"));
  void (*bf)(char*) = BfAddr.toPtr<void(char*)>();

  // TODO memory safety
  char mem[1'000'000]{};
  bf(mem+500'000);
}
