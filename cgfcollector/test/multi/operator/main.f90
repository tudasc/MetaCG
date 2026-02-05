program main
    use mod

    implicit none

    call test_compare()

contains
    subroutine test_compare()
        class(sortable), allocatable :: a, b
        type(integer_sortable) :: c, d

        c%value = 5
        d%value = 10

        allocate (a, source=c)
        allocate (b, source=d)

        if (a < b) then
            print *, "a is less then b"
        else
            print *, "a is nat less then b"
        end if
    end subroutine test_compare

end program main

