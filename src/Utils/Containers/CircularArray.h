#ifndef WD_CIRCULAR_ARRAY_H
#define WD_CIRCULAR_ARRAY_H

namespace Wado {

    template <typename T, size_t N>
    class CircularArray {
        public:
            CircularArray() noexcept { };
            ~CircularArray() noexcept { };
            
            // TODO: should I enforce power of two here and do shifts instead of modulo?
            T get(size_t index) const noexcept{
                return elements[index % N];  
            };

            void insert(size_t index, T object) {
                elements[index % N] = object;
            };

            size_t size() const noexcept {
                return N;
            };
        private:
            T elements[N];
    }; 

};

#endif