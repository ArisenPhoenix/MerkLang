extern "C" const char* __lsan_default_options() {
  return "suppressions=/absolute/path/to/lsan.supp";
}