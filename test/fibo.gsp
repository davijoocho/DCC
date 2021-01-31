
func fib (int x) => int {
    if (x <= 1) return x;
    return fib(x-2) + fib(x-1);
}

func main() => void {
    int res = fib(5);
    output(res);
}
