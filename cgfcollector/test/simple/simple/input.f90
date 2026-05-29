program main
    implicit none

    call print_stars(5)
contains
    subroutine print_stars(n)
        implicit none
        integer, intent(in) :: n
        integer :: i

        do i = 1, n
            write (*, *) '*'
        end do
    end subroutine print_stars
end program main
