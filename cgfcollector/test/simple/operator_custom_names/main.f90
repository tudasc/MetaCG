module mod

    implicit none

    type, abstract :: sortable
    contains
        procedure(not), deferred :: not_impl
        procedure(operator), deferred :: operator_impl
        procedure(weird), deferred :: weird_impl

        ! test weird operator names
        generic :: operator(.NotHInG.) => not_impl
        generic :: operator(.operator.) => operator_impl
        generic :: operator(.WeIrD.) => weird_impl
    end type sortable

    abstract interface
        logical function not(self)
            import :: sortable
            class(sortable), intent(in) :: self
        end function not

        integer function operator(self, x)
            import :: sortable
            class(sortable), intent(in) :: self
            integer, intent(in) :: x
        end function operator

        real function weird(lhs, rhs)
            import :: sortable
            class(sortable), intent(in) :: lhs
            class(sortable), intent(in) :: rhs
        end function weird
    end interface

    type, extends(sortable) :: concrete
        integer :: value = 0
    contains
        procedure :: not_impl => concrete_not
        procedure :: operator_impl => concrete_operator
        procedure :: weird_impl => concrete_weird
    end type concrete

contains

    logical function concrete_not(self)
        class(concrete), intent(in) :: self

        concrete_not = self%value == 0
    end function concrete_not

    integer function concrete_operator(self, x)
        class(concrete), intent(in) :: self
        integer, intent(in) :: x

        concrete_operator = self%value + x
    end function concrete_operator

    real function concrete_weird(lhs, rhs)
        class(concrete), intent(in) :: lhs
        class(sortable), intent(in) :: rhs

        select type (rhs)
        type is (concrete)
            concrete_weird = lhs%value*rhs%value
        class default
            concrete_weird = -1.0
        end select
    end function concrete_weird

end module mod

program main

    use mod
    implicit none

    type(concrete) :: a, b
    logical :: l
    integer :: i
    real :: r

    a%value = 3
    b%value = 7

    l = .NotHInG.a
    i = a.operator.42
    r = a.WeIrD.b

    print *, l
    print *, i
    print *, r

end program main
