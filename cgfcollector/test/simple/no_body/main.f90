program main
    implicit none

    interface
        subroutine print_stuff(n)
            implicit none
            integer, intent(in)::n
        end subroutine print_stuff
    end interface

    call print_stuff(1)

    call print_stars(5)
contains
    subroutine print_stars(n)
        implicit none
        integer, intent(in) :: n
        integer :: i

        interface
            subroutine print_stuff2(f)
                implicit none
                integer, intent(in)::f
            end subroutine print_stuff2
        end interface

        call print_stuff(1)

        do i = 1, n
            write (*, *) '*'
        end do

    contains
        subroutine subsub(d)
            integer, intent(in) :: d
        end subroutine subsub

    end subroutine print_stars
end program main
