module mod

    implicit none

    type, abstract :: base
        integer :: value
    contains
        procedure(fbase), deferred :: function_base
    end type base

    abstract interface
        logical function fbase(this)
            import :: base
            implicit none
            class(base), intent(in) :: this
        end function fbase
    end interface

    type, extends(base) :: derived
        integer :: extra_value
    contains
        procedure :: function_base => function_base2
    end type derived

    interface operator(.NOT.)
        module procedure not_base
    end interface

contains

    logical function not_base(this)
        class(base), intent(in) :: this

        select type (this)
        type is (derived)
            print *, "Derived NOT called with value: ", this%value, " and extra value: ", this%extra_value
            not_base = .true.
        class default
            print *, "Base NOT called with value: ", this%value
            not_base = .false.
        end select
    end function not_base

    logical function function_base2(this)
        class(derived), intent(in) :: this
        print *, "Derived function called with value: ", this%value, " and extra value: ", this%extra_value
        function_base2 = .true.
    end function function_base2
end module mod

program main
    use mod
    implicit none

    class(base), allocatable :: obj1, obj2, result

    class(base), allocatable :: obj3
    logical :: obj3_result, obj3_result2

    allocate (derived :: obj3)
    obj3%value = 20
    select type (d => obj3)
    type is (derived)
        d%extra_value = 30
    end select
    obj3_result = (.NOT. obj3)
    obj3_result2 = obj3%function_base()
    print *, "Base object NOT: ", obj3_result
    print *, "Base function: ", obj3_result2

end program main

