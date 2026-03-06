#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cassert>
#include <chrono>
#include <iomanip>

#include "core/TypesFWD.hpp"
#include "utilities/streaming.h"


#include "core/node/Node.hpp"
#include "core/Tokenizer.hpp"
#include "core/Parser.hpp"
#include "core/registry/Context.hpp"

#include "ast/AstBase.hpp"
#include "ast/AstControl.hpp"
#include "ast/Ast.hpp"

#include "core/Environments/Scope.hpp"
#include "core/Environments/StackScope.hpp"
#include "core/builtins.h"

#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "utilities/utilities.h"


#include "lex/Scanner.hpp"
#include "lex/Structurizer.hpp"
#include "lex/Lexer.hpp"
#include "core/evaluators/TypeEvaluator.hpp"
#include "core/evaluators/FastIR.hpp"



std::tuple<String, String> getFileContents(int argc, char* argv[], bool onlyPath) {
    const String codeDir = "code/";
    const String defaultFile = "test1.merk";

    String filePath = getFilePath(argc, argv, codeDir, defaultFile);
    DEBUG_LOG(LogLevel::DEBUG, "Using File: ", filePath);
    
    if (onlyPath) {
        String content = "";

        return std::tuple(content, filePath);
    }
    String content = readFile(filePath);
    outputFileContents(content, 800);
    return std::tuple(content, filePath);
}

SharedPtr<Scope> generateGlobalScope(bool interpretMode, LexerConfig& lCfg) {
    StackScope::resetInstrumentation();
    // StackScope(0, false,true);
    SharedPtr<Scope> globalScope = makeShared<StackScope>(0, interpretMode, true);
    globalScope->owner = "GLOBAL";
    globalScope->kind = ScopeKind::Root;

    
    auto globalFunctions = getNativeFunctions(globalScope);
    for (auto& [name, globalFunc]: globalFunctions) {
        globalScope->globalFunctions->registerFunction(name, globalFunc);
        if (globalScope->globalFunctions->getFunction(name)) {};
        const String n = name;
        lCfg.nativeFuncs.emplace(n);
    }

    auto globalClasses = getNativeClasses(globalScope);
    for (auto& [name, globalCls]: globalClasses) {
        globalScope->globalClasses->registerClass(name, globalCls);
        const String n = name;
        lCfg.nativeClasses.emplace(n);
    }

    return globalScope;
}


long fib_memoized(long n, std::unordered_map<long, long>& cache) {
        auto it = cache.find(n);
        if (it != cache.end()) {
            return it->second;
        }

        if (n <= 1) {
            cache[n] = n;
            return n;
        } else {
            long value = fib_memoized(n - 1, cache) + fib_memoized(n - 2, cache);

            cache[n] = value;
            return value;
        }
    }

void testFunc() {
    std::unordered_map<long, long> cached;

    
    auto val = fib_memoized(35, cached);

    String out = TypeEvaluator::as<String>(val);
    debugLog(true, out);
};






int run_original(int argc, char* argv[]) {
// Debug::configureDebugger();
    Debug::configureDebugger();
    auto [content, filePath] = getFileContents(argc, argv, true);
    const bool interpretMode = false;
    const bool byBlock = false;
    LexerConfig lCfg;
    SharedPtr<Scope> globalScope = generateGlobalScope(interpretMode, lCfg);
    auto logLevel = Debugger::getInstance().getLogLevel();
    // auto flowLevel = Debugger::getInstance().getFlowLevel();
    
    try {
        

        DEBUG_LOG(LogLevel::DEBUG, "Initializing tokenizer...");
        Tokenizer tokenizer(filePath, true);
        DEBUG_LOG(LogLevel::DEBUG, "Starting tokenization...");

        auto tokens = tokenizer.tokenize(lCfg);
        DEBUG_LOG(LogLevel::DEBUG, "Tokenization complete.\n");
        
        #ifdef ENABLE_DEBUG
        if (logLevel == LogLevel::INFO) {
            tokenizer.printTokens(true);
        }
        #endif
        // tokenizer.printTokens(true);
        DEBUG_LOG(LogLevel::DEBUG, "\nInitializing parser...");
        Parser parser(tokens, globalScope, interpretMode, byBlock);
        DEBUG_LOG(LogLevel::DEBUG, "\nStarting parser...");
        Timer timer = Timer();
        auto ast = parser.parse();
        
        if (!interpretMode) {
            ast->evaluateFlow(globalScope);
        }

        timer.printElapsed(filePath);
        if (filePath.find("fib.merk")!=String::npos) {
            testFunc();
        }
             

        DEBUG_LOG(LogLevel::DEBUG, "Terminating Program...");
        if (logLevel == LogLevel::PERMISSIVE) {
            DEBUG_LOG(LogLevel::DEBUG, "==================== PRINTING GLOBAL SCOPE ====================");
            globalScope->debugPrint();   
        }
        // globalScope->printScopeTree();

        ast->clear();
        
        DEBUG_LOG(LogLevel::DEBUG, "");
        DEBUG_LOG(LogLevel::DEBUG, "==================== TRY TERMINATION ====================");

    } catch (MerkError& e){
        std::cerr << e.errorString() << std::endl;
        
    } catch (const std::exception& ex) {
        std::cerr << "Error during execution: " << ex.what() << std::endl;
        return 1;
    } 
    globalScope->printVariables();

    // Print the final state of the Global Scope right before exiting
    DEBUG_LOG(LogLevel::DEBUG, "");
    DEBUG_LOG(LogLevel::DEBUG, "==================== FINAL GLOBAL SCOPE ====================");
    globalScope->clear();
    globalScope.reset();
    Scope::printScopeReport();
    StackScope::printInstrumentation();
    
    return 0;
}




struct CliOptions {
    bool benchmark = false;
    bool benchmarkEvalOnly = false;
    bool benchmarkEvalFastInt = false;
    bool nodeBenchmark = false;
    bool manualComputeBenchmark = false;
    int benchmarkIters = 10;
    int benchmarkWarmup = 1;
    String fileName = "test1.merk";
    bool showHelp = false;
};

struct RunMetrics {
    bool ok = false;
    size_t tokenCount = 0;
    double tokenizeMs = 0.0;
    double parseMs = 0.0;
    double evalMs = 0.0;
    double totalMs = 0.0;
};

class ScopedSilenceCout {
public:
    explicit ScopedSilenceCout(bool enabled) : enabled(enabled) {
        if (this->enabled) {
            oldBuf = std::cout.rdbuf(sink.rdbuf());
        }
    }
    ~ScopedSilenceCout() {
        if (enabled && oldBuf) {
            std::cout.rdbuf(oldBuf);
        }
    }
private:
    bool enabled = false;
    std::streambuf* oldBuf = nullptr;
    std::ostringstream sink;
};

static String resolveFilePath(const String& fileName) {
    const String codeDir = "code/";
    const String baseDir = "../";
    return baseDir + joinPaths(codeDir, fileName);
}

static int parsePositiveInt(const String& value, const String& optName) {
    try {
        int parsed = std::stoi(value);
        if (parsed < 1) {
            throw std::runtime_error(optName + " must be >= 1");
        }
        return parsed;
    } catch (...) {
        throw std::runtime_error("Invalid value for " + optName + ": " + value);
    }
}

static CliOptions parseCliOptions(int argc, char* argv[]) {
    CliOptions options;
    for (int i = 1; i < argc; ++i) {
        const String arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            options.showHelp = true;
            continue;
        }
        if (arg == "--bench") {
            options.benchmark = true;
            continue;
        }
        if (arg == "--bench-eval") {
            options.benchmarkEvalOnly = true;
            continue;
        }
        if (arg == "--bench-eval-fastint") {
            options.benchmarkEvalOnly = true;
            options.benchmarkEvalFastInt = true;
            continue;
        }
        if (arg == "--bench-node") {
            options.nodeBenchmark = true;
            continue;
        }
        if (arg == "--bench-manual") {
            options.manualComputeBenchmark = true;
            continue;
        }
        if (arg == "--bench-iters" && i + 1 < argc) {
            options.benchmarkIters = parsePositiveInt(argv[++i], "--bench-iters");
            continue;
        }
        if (arg.rfind("--bench-iters=", 0) == 0) {
            options.benchmarkIters = parsePositiveInt(arg.substr(String("--bench-iters=").size()), "--bench-iters");
            continue;
        }
        if (arg == "--bench-warmup" && i + 1 < argc) {
            options.benchmarkWarmup = parsePositiveInt(argv[++i], "--bench-warmup");
            continue;
        }
        if (arg.rfind("--bench-warmup=", 0) == 0) {
            options.benchmarkWarmup = parsePositiveInt(arg.substr(String("--bench-warmup=").size()), "--bench-warmup");
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            throw std::runtime_error("Unknown option: " + arg);
        }
        options.fileName = arg;
    }
    return options;
}

static void printUsage() {
    std::cout
        << "Usage:\n"
        << "  ./merk [file.merk]\n"
        << "  ./merk --bench [file.merk] [--bench-iters N] [--bench-warmup N]\n"
        << "  ./merk --bench-eval [file.merk] [--bench-iters N] [--bench-warmup N]\n"
        << "  ./merk --bench-eval-fastint [file.merk] [--bench-iters N] [--bench-warmup N]\n"
        << "  ./merk --bench-node [--bench-iters N] [--bench-warmup N]\n"
        << "  ./merk --bench-manual [--bench-iters N] [--bench-warmup N]\n";
}

struct NodeBenchMetrics {
    double isNullMs = 0.0;
    double isCallableMs = 0.0;
    double toStringMs = 0.0;
    double copyAssignMs = 0.0;
    double arithmeticMs = 0.0;
    double hashMs = 0.0;
    double totalMs = 0.0;
};

static NodeBenchMetrics runNodeMicroBenchOnce() {
    using Clock = std::chrono::steady_clock;
    NodeBenchMetrics m;

    constexpr int inner = 50000;
    volatile int sinkInt = 0;
    volatile bool sinkBool = false;
    volatile std::size_t sinkHash = 0;
    volatile std::size_t sinkLen = 0;

    Node nNull(Null);
    Node nInt(123);
    Node nBool(true);
    Node nStr(String("merk"));
    Node nOne(1);
    Node nTwo(2);

    auto tTotal0 = Clock::now();

    auto t0 = Clock::now();
    for (int i = 0; i < inner; ++i) {
        sinkBool ^= nNull.isNull();
        sinkBool ^= nInt.isNull();
    }
    auto t1 = Clock::now();
    m.isNullMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    t0 = Clock::now();
    for (int i = 0; i < inner; ++i) {
        sinkBool ^= nInt.isCallable();
        sinkBool ^= nNull.isCallable();
    }
    t1 = Clock::now();
    m.isCallableMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    t0 = Clock::now();
    for (int i = 0; i < inner; ++i) {
        sinkLen += nInt.toString().size();
        sinkLen += nBool.toString().size();
        sinkLen += nStr.toString().size();
    }
    t1 = Clock::now();
    m.toStringMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    t0 = Clock::now();
    Node a(1), b(2), c(3);
    for (int i = 0; i < inner; ++i) {
        a = b;
        b = c;
        c = a;
        sinkInt += a.toInt();
    }
    t1 = Clock::now();
    m.copyAssignMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    t0 = Clock::now();
    for (int i = 0; i < inner; ++i) {
        Node sum = nOne + nTwo;
        sinkInt += sum.toInt();
    }
    t1 = Clock::now();
    m.arithmeticMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    t0 = Clock::now();
    for (int i = 0; i < inner; ++i) {
        sinkHash ^= nInt.hash();
        sinkHash ^= nStr.hash();
    }
    t1 = Clock::now();
    m.hashMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    auto tTotal1 = Clock::now();
    m.totalMs = std::chrono::duration<double, std::milli>(tTotal1 - tTotal0).count();

    if (sinkInt == -1 && sinkBool && sinkHash == 0 && sinkLen == 0) {
        std::cerr << "Impossible sink state\n";
    }

    return m;
}

static int runNodeBenchmark(const CliOptions& options) {
    double isNullTotal = 0.0;
    double isCallableTotal = 0.0;
    double toStringTotal = 0.0;
    double copyAssignTotal = 0.0;
    double arithmeticTotal = 0.0;
    double hashTotal = 0.0;
    double overallTotal = 0.0;

    for (int i = 0; i < options.benchmarkWarmup; ++i) {
        (void)runNodeMicroBenchOnce();
    }

    for (int i = 0; i < options.benchmarkIters; ++i) {
        auto r = runNodeMicroBenchOnce();
        isNullTotal += r.isNullMs;
        isCallableTotal += r.isCallableMs;
        toStringTotal += r.toStringMs;
        copyAssignTotal += r.copyAssignMs;
        arithmeticTotal += r.arithmeticMs;
        hashTotal += r.hashMs;
        overallTotal += r.totalMs;
    }

    const double denom = static_cast<double>(options.benchmarkIters);
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "\nNode Benchmark Results\n";
    std::cout << "Iterations: " << options.benchmarkIters
              << " (warmup: " << options.benchmarkWarmup << ")\n";
    std::cout << "Avg isNull:      " << (isNullTotal / denom) << " ms\n";
    std::cout << "Avg isCallable:  " << (isCallableTotal / denom) << " ms\n";
    std::cout << "Avg toString:    " << (toStringTotal / denom) << " ms\n";
    std::cout << "Avg copyAssign:  " << (copyAssignTotal / denom) << " ms\n";
    std::cout << "Avg arithmetic:  " << (arithmeticTotal / denom) << " ms\n";
    std::cout << "Avg hash:        " << (hashTotal / denom) << " ms\n";
    std::cout << "Avg total:       " << (overallTotal / denom) << " ms\n";
    return 0;
}

struct ManualComputeMetrics {
    double rawIntMs = 0.0;
    double nodeOpMs = 0.0;
    int rawChecksum = 0;
    int nodeChecksum = 0;
};

static ManualComputeMetrics runManualComputeOnce() {
    using Clock = std::chrono::steady_clock;
    ManualComputeMetrics m;

    constexpr int n = 300000;
    constexpr int mod = 1000003;

    {
        auto t0 = Clock::now();
        int i = 0;
        int a = 1;
        int b = 2;
        int c = 0;

        while (i < n) {
            a = (a * 13 + 17) % mod;
            b = (b + a + i) % mod;

            if ((i % 7) == 0) {
                c = c + (a % 97);
            } else {
                c = c + (b % 89);
            }
            i = i + 1;
        }

        m.rawChecksum = (a + b + c + i) % mod;
        auto t1 = Clock::now();
        m.rawIntMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    }

    {
        const Node nNode(n);
        const Node modNode(mod);
        const Node mulNode(13);
        const Node addNode(17);
        const Node sevenNode(7);
        const Node ninetySevenNode(97);
        const Node eightyNineNode(89);
        const Node oneNode(1);
        const Node zeroNode(0);

        Node iNode(0);
        Node aNode(1);
        Node bNode(2);
        Node cNode(0);

        auto t0 = Clock::now();
        while (iNode < nNode) {
            aNode = ((aNode * mulNode) + addNode) % modNode;
            bNode = ((bNode + aNode) + iNode) % modNode;

            if ((iNode % sevenNode) == zeroNode) {
                cNode = cNode + (aNode % ninetySevenNode);
            } else {
                cNode = cNode + (bNode % eightyNineNode);
            }

            iNode = iNode + oneNode;
        }

        Node checksum = (((aNode + bNode) + cNode) + iNode) % modNode;
        m.nodeChecksum = checksum.toInt();
        auto t1 = Clock::now();
        m.nodeOpMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    }

    return m;
}

static int runManualComputeBenchmark(const CliOptions& options) {
    double rawTotal = 0.0;
    double nodeTotal = 0.0;
    int rawChecksum = 0;
    int nodeChecksum = 0;

    for (int i = 0; i < options.benchmarkWarmup; ++i) {
        (void)runManualComputeOnce();
    }

    for (int i = 0; i < options.benchmarkIters; ++i) {
        auto r = runManualComputeOnce();
        rawTotal += r.rawIntMs;
        nodeTotal += r.nodeOpMs;
        rawChecksum = r.rawChecksum;
        nodeChecksum = r.nodeChecksum;
    }

    const double denom = static_cast<double>(options.benchmarkIters);
    const double rawAvg = rawTotal / denom;
    const double nodeAvg = nodeTotal / denom;
    const double ratio = (rawAvg > 0.0) ? (nodeAvg / rawAvg) : 0.0;

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "\nManual Compute Benchmark Results\n";
    std::cout << "Kernel: compute_loop equivalent (while-only arithmetic)\n";
    std::cout << "Iterations: " << options.benchmarkIters
              << " (warmup: " << options.benchmarkWarmup << ")\n";
    std::cout << "Avg raw-int:    " << rawAvg << " ms\n";
    std::cout << "Avg node-op:    " << nodeAvg << " ms\n";
    std::cout << "Node/raw ratio: " << ratio << "x\n";
    std::cout << "Checksums: raw=" << rawChecksum << ", node=" << nodeChecksum << "\n";

    return 0;
}

static RunMetrics runPipelineOnce(const String& filePath, bool printScopeReport) {
    using Clock = std::chrono::steady_clock;
    RunMetrics metrics;
    const bool interpretMode = false;
    const bool byBlock = false;
    LexerConfig lCfg;
    SharedPtr<Scope> globalScope = generateGlobalScope(interpretMode, lCfg);
    auto logLevel = Debugger::getInstance().getLogLevel();

    auto tTotalStart = Clock::now();

    try {
        Tokenizer tokenizer(filePath, true);

        auto t0 = Clock::now();
        auto tokens = tokenizer.tokenize(lCfg);
        auto t1 = Clock::now();
        metrics.tokenizeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        metrics.tokenCount = tokens.size();

#ifdef ENABLE_DEBUG
        if (logLevel == LogLevel::DEBUG) {
            tokenizer.printTokens(true);
        }
#endif

        t0 = Clock::now();
        Parser parser(tokens, globalScope, interpretMode, byBlock);
        auto ast = parser.parse();
        t1 = Clock::now();
        metrics.parseMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

        t0 = Clock::now();
        if (!interpretMode) {
            ast->evaluateFlow(globalScope);
        }
        t1 = Clock::now();
        metrics.evalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

        ast->clear();
        metrics.ok = true;
    } catch (MerkError& e) {
        std::cerr << e.errorString() << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Error during execution: " << ex.what() << std::endl;
    }

    globalScope->clear();
    globalScope.reset();
    if (printScopeReport) {
        Scope::printScopeReport();
        StackScope::printInstrumentation();
    }

    auto tTotalEnd = Clock::now();
    metrics.totalMs = std::chrono::duration<double, std::milli>(tTotalEnd - tTotalStart).count();
    return metrics;
}

static int runBenchmark(const CliOptions& options) {
    const String filePath = resolveFilePath(options.fileName);

    double totalTok = 0.0;
    double totalParse = 0.0;
    double totalEval = 0.0;
    double totalTotal = 0.0;
    size_t tokens = 0;

    {
        ScopedSilenceCout silence(true);

        for (int i = 0; i < options.benchmarkWarmup; ++i) {
            auto warmup = runPipelineOnce(filePath, false);
            if (!warmup.ok) {
                return 1;
            }
        }

        for (int i = 0; i < options.benchmarkIters; ++i) {
            auto run = runPipelineOnce(filePath, false);
            if (!run.ok) {
                return 1;
            }
            totalTok += run.tokenizeMs;
            totalParse += run.parseMs;
            totalEval += run.evalMs;
            totalTotal += run.totalMs;
            tokens = run.tokenCount;
        }
    }

    const double denom = static_cast<double>(options.benchmarkIters);
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "\nBenchmark Results\n";
    std::cout << "File: " << filePath << "\n";
    std::cout << "Iterations: " << options.benchmarkIters
              << " (warmup: " << options.benchmarkWarmup << ")\n";
    std::cout << "Tokens: " << tokens << "\n";
    std::cout << "Avg tokenize: " << (totalTok / denom) << " ms\n";
    std::cout << "Avg parse:    " << (totalParse / denom) << " ms\n";
    std::cout << "Avg eval:     " << (totalEval / denom) << " ms\n";
    std::cout << "Avg total:    " << (totalTotal / denom) << " ms\n";
    return 0;
}

// Parse once, eval many times (fresh scope per eval). Comparable to Python's compile-once-exec-many.
static int runBenchmarkEvalOnly(int argc, char* argv[], const CliOptions& options) {
    const String codeDir = "code/";
    std::cout << "codeDir: " << codeDir;
    const String filePath = getFilePath(argc, argv, codeDir, options.fileName);
    const bool interpretMode = false;
    const bool byBlock = false;
    LexerConfig lCfg;
    SharedPtr<Scope> parseScope = generateGlobalScope(interpretMode, lCfg);

    UniquePtr<CodeBlock> ast;
    size_t tokenCount = 0;
    FastIR::Program fastProgram;
    bool fastProgramReady = false;

    try {
        Tokenizer tokenizer(filePath, true);
        auto tokens = tokenizer.tokenize(lCfg);
        tokenCount = tokens.size();

        Parser parser(tokens, parseScope, interpretMode, byBlock);
        ast = parser.parse();
        if (options.benchmarkEvalFastInt) {
            fastProgramReady = FastIR::lowerCodeBlock(*ast, fastProgram);
        }
    } catch (MerkError& e) {
        std::cerr << e.errorString() << std::endl;
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Error during parse: " << ex.what() << std::endl;
        return 1;
    }

    double totalEval = 0.0;
    {
        ScopedSilenceCout silence(true);

        for (int i = 0; i < options.benchmarkWarmup; ++i) {
            SharedPtr<Scope> scope = parseScope->clone();
            if (fastProgramReady) {
                FastIR::execute(fastProgram, scope);
            } else {
                ast->evaluateFlow(scope);
            }
            scope->clear();
            scope.reset();
        }

        for (int i = 0; i < options.benchmarkIters; ++i) {
            SharedPtr<Scope> scope = parseScope->clone();

            auto t0 = std::chrono::steady_clock::now();
            if (fastProgramReady) {
                FastIR::execute(fastProgram, scope);
            } else {
                ast->evaluateFlow(scope);
            }
            auto t1 = std::chrono::steady_clock::now();

            totalEval += std::chrono::duration<double, std::milli>(t1 - t0).count();
            scope->clear();
            scope.reset();
        }
    }

    ast->clear();
    ast.reset();
    parseScope->clear();
    parseScope.reset();

    const double denom = static_cast<double>(options.benchmarkIters);
    const double avgEval = totalEval / denom;
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "\nBenchmark Results (eval-only, parse once)\n";
    std::cout << "File: " << filePath << "\n";
    std::cout << "Iterations: " << options.benchmarkIters
              << " (warmup: " << options.benchmarkWarmup << ")\n";
    std::cout << "Tokens: " << tokenCount << "\n";
    if (options.benchmarkEvalFastInt) {
        std::cout << "FastIR:       " << (fastProgramReady ? "enabled" : "fallback (unsupported AST)") << "\n";
        if (!fastProgramReady && !fastProgram.unsupportedReason.empty()) {
            std::cout << "FastIR note:  " << fastProgram.unsupportedReason << "\n";
        }
    }
    std::cout << "Avg eval:     " << avgEval << " ms\n";
    std::cout << "Avg total:    " << avgEval << " ms\n";
    return 0;
}

int main(int argc, char* argv[]) {
    Debug::configureDebugger();

    try {
        const CliOptions options = parseCliOptions(argc, argv);
        if (options.showHelp) {
            printUsage();
            return 0;
        }
        if (options.benchmark) {
            return runBenchmark(options);
        }
        if (options.benchmarkEvalOnly) {
            return runBenchmarkEvalOnly(argc, argv, options);
        }
        if (options.nodeBenchmark) {
            return runNodeBenchmark(options);
        }
        if (options.manualComputeBenchmark) {
            return runManualComputeBenchmark(options);
        }
        return run_original(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        printUsage();
        return 1;
    }
}






