module mod
    implicit none

    type :: res_a
        integer :: id
    contains
        final :: cleanup_a
    end type res_a

    type :: res_b
        integer :: id
    contains
        final :: cleanup_b
    end type res_b

    interface setup
        module procedure setup_a, setup_b
    end interface

contains
    subroutine cleanup_a(this)
        type(res_a), intent(inout) :: this
        print *, "cleanup a"
    end subroutine cleanup_a

    subroutine cleanup_b(this)
        type(res_b), intent(inout) :: this
        print *, "cleanup b"
    end subroutine cleanup_b

    ! does NOT initialize its argument
    subroutine setup_a(r)
        type(res_a), allocatable, intent(inout) :: r
    end subroutine setup_a

    ! initializes its argument
    subroutine setup_b(r)
        type(res_b), allocatable, intent(inout) :: r
        allocate (r)
        r%id = 1
        print *, "setup_b does nothing"
    end subroutine setup_b
end module mod

program main
    use mod
    implicit none

    call test_a()
    call test_b()

contains
    subroutine test_a()
        type(res_a), allocatable :: x
        call setup(x)
    end subroutine test_a

    subroutine test_b()
        type(res_b), allocatable :: y
        call setup(y)
    end subroutine test_b
end program main
