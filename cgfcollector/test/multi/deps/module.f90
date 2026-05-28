module my_module
    use module0_5

    implicit none

contains
    subroutine subsub()
        integer :: n
    end subroutine subsub

    subroutine my_subroutine(n)
        integer, intent(in) :: n

        write (*, *) n

        call func()

    end subroutine my_subroutine
end module my_module

