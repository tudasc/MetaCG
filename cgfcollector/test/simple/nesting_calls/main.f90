module m
    implicit none

    type :: T
    contains
        procedure :: func => return_ptr
        procedure :: say
    end type T

contains

    function return_ptr(this) result(p)
        class(T), intent(in) :: this
        class(T), pointer :: p
        allocate (T :: p)
        select type (p)
        type is (T)
            p = this
        end select
    end function return_ptr

    subroutine say(this)
        class(T), intent(in) :: this
        print *, 'Hello from say'
    end subroutine say

end module m

program main
    use m
    implicit none

    type(T) :: a
    class(T), pointer :: tmp

    tmp => a%func()
    call tmp%say()
end program main
