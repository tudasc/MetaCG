module example_mod
    implicit none

    type :: Inner
    contains
        procedure :: hello
    end type Inner

    type :: Outer
        type(Inner) :: obj
    end type Outer

contains

    subroutine hello(this)
        class(Inner), intent(inout) :: this

        print *, "Hello from the inner object!"
    end subroutine hello

end module example_mod

program test
    use example_mod
    implicit none

    type(Outer) :: obj

    call obj%obj%hello()

end program test
