#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

        
        long long a, b;
        while (std::cin >> a >> b) {
            // salida: 0 0
            if (a == 0 && b == 0) {
                break;
            }        long long s = a + b;
        std::cout << s << '\n' << std::flush;
    }

    return 0;
}

