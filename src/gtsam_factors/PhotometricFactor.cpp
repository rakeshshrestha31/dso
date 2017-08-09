//
// Created by sicong on 08/08/17.
//

#include "PhotometricFactor.h"

PhotometricFactor::PhotometricFactor(   Key j, CoarseTracker *coarseTracker,
                                        int lvl, int pointIdx,
                                        const SharedNoiseModel& model
                                    )
    : NoiseModelFactor1<Pose3>(model, j),
      coarseTracker_(coarseTracker),
      lvl_(lvl), pointIdx_(pointIdx)
{

}

Vector PhotometricFactor::evaluateError (    const Pose3& pose,
                                             boost::optional<Matrix&> H
                                        ) const
{
    if (H)
    {
        SE3 Tib(pose.matrix());
        SE3 Tw_reffromNAV = SE3(coarseTracker_->lastRef->shell->navstate.pose().matrix()) *
                            coarseTracker_->imutocam().inverse();

        SE3 Tw_ref = coarseTracker_->lastRef->shell->camToWorld;
        Mat33 Rcb = coarseTracker_->imutocam().rotationMatrix();
        SE3 Trb = Tw_ref.inverse() * Tib;

        Vec3 Pr,PI;
        Vec6 Jab;
        Vec2 dxdy;
        dxdy(0) = *(coarseTracker_->buf_warped_dx + pointIdx_);
        dxdy(1)	= *(coarseTracker_->buf_warped_dy + pointIdx_);
//        float b0 = lastRef_aff_g2l.b;
//        float a = (float)(AffLight::fromToVecExposure(coarseTracker_->lastRef->ab_exposure, coarseTracker_->newFrame->ab_exposure, lastRef_aff_g2l, aff_g2l)[0]);

        float id = *(coarseTracker_->buf_warped_idepth + pointIdx_);
        float u = *(coarseTracker_->buf_warped_u + pointIdx_);
        float v = *(coarseTracker_->buf_warped_v + pointIdx_);
        Pr(0) = *(coarseTracker_->buf_warped_rx + pointIdx_);
        Pr(1) = *(coarseTracker_->buf_warped_ry + pointIdx_);
        Pr(2) = *(coarseTracker_->buf_warped_rz + pointIdx_);
        PI = Tw_ref * Pr;
        // Jacobian of camera projection
        Matrix23 Maux;
        Maux.setZero();
        Maux(0,0) = coarseTracker_->fx[lvl_];
        Maux(0,1) = 0;
        Maux(0,2) = -u *coarseTracker_->fx[lvl_];
        Maux(1,0) = 0;
        Maux(1,1) = coarseTracker_->fy[lvl_];
        Maux(1,2) = -v * coarseTracker_->fy[lvl_];

        Matrix23 Jpi = Maux * id;

        Jab.head(3) = dxdy.transpose() * Jpi * Rcb * SO3::hat(Tib.inverse() * PI); //Rrb.transpose()*(Pr-Prb));
        Jab.tail(3) = dxdy.transpose() * Jpi * (-Rcb);


        *H = Jab.transpose();
    }
    return (Vector(1) << -coarseTracker_->buf_warped_residual[pointIdx_]).finished();
}