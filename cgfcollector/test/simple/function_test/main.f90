module vector_operations
contains
    function vector_add(a, b) result(result)
        implicit none
        real, dimension(:), intent(in) :: a, b
        real, dimension(size(a)) :: result
        integer :: i

        if (size(a) /= size(b)) then
            print *, "Error: Vectors must be of the same size."
            stop
        end if

        do i = 1, size(a)
            result(i) = a(i) + b(i)
        end do
    end function vector_add
    function vector_norm(n, vec) result(norm)
        implicit none
        integer, intent(in) :: n
        real, intent(in) :: vec(n)
        real :: norm

        norm = sqrt(sum(vec**2))

    end function vector_norm
end module vector_operations

function vector_norm2(n, vec) result(norm)
    implicit none
    integer, intent(in) :: n
    real, intent(in) :: vec(n)
    real :: norm

    norm = sqrt(sum(vec**2))

end function vector_norm2

function size(arr) result(s)
    implicit none
    real, dimension(:), intent(in) :: arr
    integer :: s

    s = 10

contains
    function func1(arr) result(s)
        implicit none
        real, dimension(:), intent(in) :: arr
        integer :: s

        s = size(arr)
    end function func1
    subroutine func2(arr)
        implicit none
        real, dimension(:), intent(in) :: arr
        integer :: s

        s = size(arr)
    end subroutine func2
end function size

program main
    use vector_operations, only: vector_add, vector_norm2 => vector_norm
    implicit none

    real, dimension(3) :: a, b, result
    integer :: i

    interface
        function size(arr) result(s)
            implicit none
            real, dimension(:), intent(in) :: arr
            integer :: s
        end function size
    end interface

    a = [1.0, 2.0, 3.0]
    b = [4.0, 5.0, 6.0]
    result = vector_add(a, b)

    print *, "Result of vector addition:"
    do i = 1, size(result)
        print *, result(i)
    end do

    i = size(a)
    print *, "i: ", i

    print *, "Norm of vector a:"
    print *, vector_norm2(size(a), a)
end program main
