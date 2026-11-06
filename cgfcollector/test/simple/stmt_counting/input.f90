module math_utils
    implicit none
    private
    public :: factorial, print_array

contains

    recursive function factorial(n) result(res)
        integer, intent(in) :: n
        integer :: res

        if (n <= 1) then
            res = 1
        else
            res = n*factorial(n - 1)
        end if
    end function factorial

    subroutine print_array(a)
        integer, intent(in) :: a(:)
        integer :: i

        do i = 1, size(a)
            write (*, '(I4)', advance='no') a(i)
        end do
        write (*, *)
    end subroutine print_array

end module math_utils

program statement_counter_test
    use math_utils
    implicit none

    integer, parameter :: N = 8
    integer :: arr(N)
    integer :: i, total, ios
    logical :: flag
    character(len=20) :: text

    arr = [(i, i=1, N)]
    total = 0
    flag = .true.
    text = "Fortran"

    do i = 1, N
        total = total + arr(i)
    end do

    if (total > 50) then
        write (*, *) "Large"
    else if (total > 20) then
        write (*, *) "Medium"
    else
        write (*, *) "Small"
    end if

    select case (mod(total, 4))
    case (0)
        write (*, *) "Divisible by 4"
    case (1)
        write (*, *) "Remainder 1"
    case (2:3)
        write (*, *) "Remainder 2 or 3"
    case default
        write (*, *) "Impossible"
    end select

    where (arr > 4)
        arr = arr*10
    elsewhere
        arr = -arr
    end where

    call print_array(arr)

    forall (i=1:N, arr(i) < 0)
        arr(i) = abs(arr(i))
    end forall

    i = 1
    do while (i <= N)
        arr(i) = arr(i) + 1
        i = i + 1
    end do

    outer: do i = 1, N
        if (mod(i, 2) == 0) cycle outer
        if (i > 6) exit outer
        write (*, *) "Odd:", i
    end do outer

    block
        integer :: temp
        temp = factorial(5)
        write (*, *) "5! =", temp
    end block

    associate (first => arr(1), last => arr(N))
        write (*, *) first, last
    end associate

    read (*, *, iostat=ios) i
    if (ios /= 0) then
        write (*, *) "Input error"
    else
        write (*, *) "Input:", i
    end if

    text = trim(text)//" Test"
    write (*, *) len_trim(text), text

    write (*, *) (arr(i), i=1, N)

    write (*, *) "Max:", maxval(arr)
    write (*, *) "Min:", minval(arr)
    write (*, *) "Sum:", sum(arr)

    call demo_subroutine(arr)

contains

    subroutine demo_subroutine(x)
        integer, intent(inout) :: x(:)
        integer :: j

        do concurrent(j=1:size(x))
            x(j) = x(j)*2
        end do

        call internal_helper(x)
    end subroutine demo_subroutine

    subroutine internal_helper(y)
        integer, intent(in) :: y(:)

        if (all(y >= 0)) then
            write (*, *) "All non-negative."
        end if

        if (any(y > 20)) then
            write (*, *) "Contains values > 20."
        end if
    end subroutine internal_helper

end program statement_counter_test
