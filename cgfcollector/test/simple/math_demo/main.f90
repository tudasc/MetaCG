module math_utils
    implicit none
    private
    public :: factorial, array_stats, dot_product_custom, normalize_vector

contains
    recursive function factorial(n) result(fact)
        integer, intent(in) :: n
        integer :: fact
        if (n <= 1) then
            fact = 1
        else
            fact = n*factorial(n - 1)
        end if
    end function factorial

    subroutine array_stats(arr, mean, std_dev)
        real, intent(in) :: arr(:)
        real, intent(out) :: mean, std_dev
        real :: sum_arr, variance
        integer :: n
        n = size(arr)
        sum_arr = sum(arr)
        mean = sum_arr/n
        variance = sum((arr - mean)**2)/n
        std_dev = sqrt(variance)
    end subroutine array_stats

    function dot_product_custom(a, b) result(dp)
        real, intent(in) :: a(:), b(:)
        real :: dp
        dp = sum(a*b)
    end function dot_product_custom

    subroutine normalize_vector(vec, norm_vec)
        real, intent(in) :: vec(:)
        real, intent(out) :: norm_vec(size(vec))
        real :: magnitude
        magnitude = sqrt(sum(vec**2))
        if (magnitude /= 0.0) then
            norm_vec = vec/magnitude
        else
            norm_vec = 0.0
        end if
    end subroutine normalize_vector

end module math_utils

program complex_demo
    use math_utils, only: factorial, array_stats, dot_product_custom, normalize_vector
    implicit none

    real :: x(5), mean, std_dev, dp
    real :: vec1(3), vec2(3), norm_vec(3)
    integer :: i, fact

    x = [1.0, 2.0, 3.0, 4.0, 5.0]
    call array_stats(x, mean, std_dev)
    print *, 'Mean:', mean, 'Std Dev:', std_dev

    vec1 = [1.0, 0.0, 0.0]
    vec2 = [0.0, 1.0, 0.0]
    dp = dot_product_custom(vec1, vec2)
    print *, 'Dot Product:', dp

    call normalize_vector(vec1, norm_vec)
    print *, 'Normalized Vector:', norm_vec

    do i = 1, 5
        fact = factorial(i)
        print *, 'Factorial(', i, ') = ', fact
    end do
end program complex_demo
