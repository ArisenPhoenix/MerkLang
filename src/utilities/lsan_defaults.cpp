extern "C" const char* __lsan_default_options() {
  // You can also add: "detect_leaks=1:log_threads=1" etc.
  return "suppressions=/absolute/path/to/lsan.supp";
}