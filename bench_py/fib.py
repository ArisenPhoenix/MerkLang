cache = {}


def fib_memoized(n):
    val = cache.get(n)
    if val is not None:
        return val

    if n <= 1:
        cache[n] = n
        return n

    value = fib_memoized(n - 1) + fib_memoized(n - 2)
    cache[n] = value
    return value


times = 30
result = fib_memoized(times)
