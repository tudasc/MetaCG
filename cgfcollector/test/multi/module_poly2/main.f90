program main
    use mod_c
    implicit none
    call test_poly()
contains
    subroutine test_poly()
        class(base), allocatable :: x
        allocate (end :: x)
        call x%op()
    end subroutine test_poly
end program main
