module m
    implicit none

    type :: inner
    contains
        procedure :: say
    end type inner

    type :: outer
        type(inner) :: b
    end type outer

contains

    subroutine say(this)
        class(inner), intent(in) :: this
        print *, "Hello from inner"
    end subroutine say

end module m

program main
    use m
    implicit none

    type(outer) :: a

    call a%b%say()
end program main
