
func factorial (int x) => int {
    if (x == 1) return 1;
    return x * factorial(x-1);
}

func main() => void {
    int res = factorial(5);
    output(res);
}




