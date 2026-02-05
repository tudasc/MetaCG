module my_module

    implicit none

contains
    subroutine subsub()
        integer :: n
    end subroutine subsub

    subroutine my_subroutine(n)
        integer, intent(in) :: n

        write (*, *) n

    end subroutine my_subroutine
end module my_module

