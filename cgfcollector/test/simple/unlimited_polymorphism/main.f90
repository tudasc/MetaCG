module mod

    implicit none

    type :: my_type
        integer :: a
        real :: b
    contains
        final :: finalize_my_type
        procedure :: print_stuff
    end type my_type

    type :: my_type2
    contains
        procedure :: print_stuff => print_stuff2
        final :: finalize_my_type2
    end type my_type2

contains

    subroutine finalize_my_type(this)
        type(my_type), intent(inout) :: this
        print *, "Finalizing my_type"
    end subroutine finalize_my_type

    subroutine finalize_my_type2(this)
        type(my_type2), intent(inout) :: this
        print *, "Finalizing my_type2"
    end subroutine finalize_my_type2

    subroutine print_stuff(this)
        class(my_type), intent(in) :: this
        print *, "stuff"
    end subroutine print_stuff

    subroutine print_stuff2(this)
        class(my_type2), intent(in) :: this
        print *, "stuff"
    end subroutine print_stuff2
end module mod

program main
    use mod

    implicit none

    call func()

contains
    subroutine func()
        class(*), allocatable :: obj

        ! allocate (obj, mold=my_type2())
        ! deallocate (obj)
        ! allocate (my_type2 :: obj)
        ! deallocate (obj)
        ! allocate (obj, source=my_type2())

        ! can be assigned with allocate, move_alloc, = operator and in function arguments
        obj = my_type(123, 12)

        select type (obj)
        type is (my_type)
            call obj%print_stuff()
            print *, 'Object is of type my_type2'
        class default
            print *, 'Object is of an unknown type'
        end select
    end subroutine func
end program main

