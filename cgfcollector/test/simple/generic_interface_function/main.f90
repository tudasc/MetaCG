module m

    implicit none

    type :: string
        character(len=16) :: chars
    end type string

    interface len
        module procedure len_s
    end interface

contains

    function len_s(expr) result(length)
        implicit none
        type(string), intent(in) :: expr
        integer                  :: length

        length = len_trim(expr%chars)
    end function len_s

    subroutine assign_s_to_c(var, expr)
        implicit none
        character(*), intent(out) :: var
        type(string), intent(in)  :: expr
        integer                   :: i

        i = len(var)
    end subroutine assign_s_to_c

end module m

program main

    use m
    implicit none

end program main
